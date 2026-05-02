FROM gcc:latest
WORKDIR /app
COPY . .
RUN g++ -o server main.cpp -pthread
EXPOSE 8080
CMD ["./server"]
