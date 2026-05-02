# 用带C++编译器的镜像
FROM gcc:latest

# 设置工作目录
WORKDIR /app

# 把你的代码复制到容器里
COPY . .

# 编译你的C++服务端（注意：main.cpp要和你的文件名一致）
RUN g++ -o server main.cpp -pthread

# 暴露你代码里监听的端口（比如8888）
EXPOSE 8888

# 启动服务端
CMD ["./server"]
