This is a worker that calculates fatigue, i.e. the amount of physical wear on the press.
It does one simple, stateless task, meant to scale horizontally.
It does this once and then it's done.

Data used for testing:

```
grpcurl -plaintext -proto fatigue.proto -import-path . -d '{
"samples": [
{"position": 0,  "velocity": 50},
{"position": 10, "velocity": 50},
{"position": 20, "velocity": 50},
{"position": 20, "velocity": 0},
{"position": 20, "velocity": -50},
{"position": 10, "velocity": -50},
{"position": 0,  "velocity": -50}
]
}' localhost:50052 fatigue_worker.FatigueService/ProcessBatch
```
Result should be 400.

```
grpcurl -plaintext --emit-defaults -proto fatigue.proto -import-path . -d '{
"samples": [
{"position": 0,  "velocity": 50},
{"position": 10, "velocity": 50},
{"position": 20, "velocity": 50},
{"position": 30, "velocity": 50},
{"position": 40, "velocity": 50},
{"position": 50, "velocity": 50},
{"position": 60, "velocity": 50}
]
}' localhost:50052 fatigue_worker.FatigueService/ProcessBatch
```
Result should be 0.

```
grpcurl -plaintext --emit-defaults  -proto fatigue.proto -import-path . -d '{
"samples": []
}' localhost:50052 fatigue_worker.FatigueService/ProcessBatch
```
Result should be 0.

```
grpcurl -plaintext --emit-defaults  -proto fatigue.proto -import-path . -d '{
"samples": [
{"position": 20,  "velocity": 50},
{"position": 30, "velocity": 50},
{"position": 30, "velocity": 0},
{"position": 30, "velocity": -50},
{"position": 20, "velocity": -50}
]
}' localhost:50052 fatigue_worker.FatigueService/ProcessBatch
```
Result should be 100.



grpcurl -plaintext --emit-defaults  -proto fatigue.proto -import-path . -d '{
"samples": [
{"position": 30, "velocity": 0},
{"position": 30, "velocity": 0},
{"position": 20, "velocity": -50},
{"position": 10, "velocity": -50},
{"position": 0, "velocity": -50}
]
}' localhost:50052 fatigue_worker.FatigueService/ProcessBatch

Result should be 0.


TODO: Write a unit test for this
