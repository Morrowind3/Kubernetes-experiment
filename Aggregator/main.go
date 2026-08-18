package main

import (
	"context"
	"fmt"
	"log"
	"os"
	"os/signal"
	"sync"
	"syscall"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/emptypb"

	fatigue "Kubernetes2/Aggregator/protobuf/fatigue"
	press "Kubernetes2/Aggregator/protobuf/press"
)

func getEnvOrDefault(key, defaultVal string) string {
	var envVal, found = os.LookupEnv(key)
	if found {
		return envVal
	} else {
		return defaultVal
	}
}

const (
	batchSize           = 20
	batchChannelBuffer  = 10
	numWorkerGoroutines = 4
)

var (
	pressSimAddress      = getEnvOrDefault("PRESS_SIM_ADDRESS", "localhost:50051")
	fatigueWorkerAddress = getEnvOrDefault("FATIGUE_WORKER_ADDRESS", "localhost:50052")
)

// Read the metrics stream from RPC and accumulate them into batches to be sent to the batch channel.
func streamMetrics(ctx context.Context, client press.PressSimServiceClient, batchChan chan<- *fatigue.PressDataBatch) {
	defer close(batchChan)

	stream, err := client.FetchMetrics(ctx, &emptypb.Empty{})
	if err != nil {
		log.Printf("Failed to open metrics stream: %v", err)
		return
	}

	currentBatch := &fatigue.PressDataBatch{}
	for {
		metrics, err := stream.Recv()
		if err != nil {
			if status.Code(err) == codes.Canceled {
				log.Println("metrics stream closed: shutting down")
			} else {
				log.Printf("metrics stream error: %v", err)
			}
			return
		}

		sample := &fatigue.PressDataSample{
			Position: metrics.GetPosition(),
			Velocity: metrics.GetVelocity(),
		}
		currentBatch.Samples = append(currentBatch.Samples, sample)

		if len(currentBatch.Samples) >= batchSize {
			batchChan <- currentBatch
			currentBatch = &fatigue.PressDataBatch{}
		}
	}
}

// Pull metric batches and assign to a worker, then put the calculated wear on the result channel
func fatigueWorker(ctx context.Context, client fatigue.FatigueServiceClient, batchChan <-chan *fatigue.PressDataBatch, resultChan chan<- float32) {
	for batch := range batchChan {
		response, err := client.ProcessBatch(ctx, batch)
		if err != nil {
			if status.Code(err) == codes.Canceled {
				log.Println("fatigue worker call cancelled: shutting down")
			} else {
				log.Printf("error processing batch: %v", err)
			}
			continue
		}
		resultChan <- response.GetBatchWear()
	}
}

// The tracker is the owner of the accumulated total.
// Other goroutines may only mutate it via channels
// These channels prevent data races as channels internally queue them up.
func totalTracker(resultChan <-chan float32) {
	var total float64

	for wear := range resultChan {
		total += float64(wear)
		log.Printf("Running total wear: %f", total)
	}

	fmt.Printf("final total wear: %f\n", total)
}

func main() {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		<-sigChan
		cancel()
	}()

	pressConn, err := grpc.NewClient(pressSimAddress, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		log.Fatalf("Failed to connect to press simulator: %v", err)
	}
	defer pressConn.Close()

	fatigueConn, err := grpc.NewClient(fatigueWorkerAddress, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		log.Fatalf("Failed to connect to fatigue worker: %v", err)
	}
	defer fatigueConn.Close()

	pressClient := press.NewPressSimServiceClient(pressConn)
	fatigueClient := fatigue.NewFatigueServiceClient(fatigueConn)

	batchChan := make(chan *fatigue.PressDataBatch, batchChannelBuffer)
	resultChan := make(chan float32)

	go streamMetrics(ctx, pressClient, batchChan)

	var workerWg sync.WaitGroup
	for range numWorkerGoroutines {
		workerWg.Go(func() {
			fatigueWorker(ctx, fatigueClient, batchChan, resultChan)
		})
	}

	//Close the result channel once every worker goroutine finishes (which they will once the batches stop)
	go func() {
		workerWg.Wait()
		close(resultChan)
	}()

	//Runs until the results channel finishes
	totalTracker(resultChan)
	log.Println("aggregator shutting down")
}
