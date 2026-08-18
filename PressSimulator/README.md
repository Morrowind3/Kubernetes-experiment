This is a simple, unrealistic simulated hydraulic press that moves up and down.
Its purpose is having enough data to do something interesting in kubernetes. 
The press can be configured with environment variables.

The data can be fetched using gRPC. 
Try it with the following commands while the docker container is running on your PC:

grpcurl -plaintext -proto press.proto -import-path . localhost:50051 press_simulator.PressSimService/FetchMetrics
grpcurl -plaintext -proto press.proto -import-path . localhost:50051 press_simulator.PressSimService/FetchConfig

Sugoi, ne?!