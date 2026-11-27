# syntax=docker/dockerfile:1

FROM gcc:13-bookworm AS build
WORKDIR /src
COPY . .
RUN make && make test

FROM debian:bookworm-slim
RUN useradd --system --uid 10001 --no-create-home leviathan-log
COPY --from=build /src/bin/leviathan-log /usr/local/bin/leviathan-log
USER 10001
ENTRYPOINT ["/usr/local/bin/leviathan-log"]
CMD ["--help"]
