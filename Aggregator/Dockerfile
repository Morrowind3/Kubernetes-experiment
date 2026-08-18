# ---- Build stage ----
FROM golang:1.26.5-alpine AS build

WORKDIR /src

# Cache module downloads separately from source so code changes don't invalidate this layer
COPY go.mod go.sum ./
RUN go mod download

COPY . .

# We don't need CGO for gRPC-Go, so we can explicitly tell Go not to use C libs (CGO_enabled=0)
# this allows us to use a distroless image: safer and smaller.
RUN CGO_ENABLED=0 go build -o /out/aggregator .

# ---- Runtime stage ----
FROM gcr.io/distroless/static-debian12 AS runtime

COPY --from=build /out/aggregator /usr/local/bin/aggregator

EXPOSE 50053

# Important: use exec form, not shell form (without [""]). If k8s sends a SIGTERM, it goes to pid 1.
# pid 1 should be our application, not the shell!
ENTRYPOINT ["/usr/local/bin/aggregator"]