# syntax=docker/dockerfile:1

FROM gcc:13-bookworm AS build
WORKDIR /src
COPY . .
RUN make && make test

FROM debian:bookworm-slim
RUN useradd --system --uid 10001 --no-create-home tinyfs
COPY --from=build /src/bin/tinyfs /usr/local/bin/tinyfs
USER 10001
ENTRYPOINT ["/usr/local/bin/tinyfs"]
CMD ["--help"]
