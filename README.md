# dependencies 
* go 
* grpc and grpc procgen for Go and *C++
* Docker
* kubectl
* A kubernetes cluster e.g. Kind

# Run instructions for Kind 

1. Create a cluster for this project if needed*
```
kind create cluster --name press-kluster
```

*if you already created the cluster, make sure it's the active context with 
```
kubectl config use-context kind-<clustername>
```
or apply it as an argument with --context in step 3

2. load the docker images into the cluster*
```
kind load docker-image k8s/press:latest --name press-kluster &&
kind load docker-image k8s/fatigue:latest --name press-kluster &&
kind load docker-image k8s/aggregator:latest --name press-kluster
```
*Make sure the images exist. Build them with build.sh or manually

3. apply the kubernetes manifests. 
```
kubectl apply -f Aggregator/aggregator-manifest.yaml &&
kubectl apply -f FatigueWorker/fatigue-manifest.yaml &&
kubectl apply -f PressSimulator/press-manifest.yaml
```

4. verify it's running
```
kubectl get pods
```
Should show 1 aggregator, 1 press sim and 3 fatigue workers.

To see it working, read the logs. The press prints its movement and the aggregator prints the total fatigue. 

```
kubectl logs -f <pod name>
```

or wait for me to finish the observability layer :P
