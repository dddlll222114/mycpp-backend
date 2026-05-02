# 基础镜像：带 Maven 的 Java 17 环境
FROM maven:3.8.5-openjdk-17 AS build

# 设置工作目录
WORKDIR /app

# 复制项目文件
COPY pom.xml .
COPY src ./src

# 编译打包项目，生成 jar 包
RUN mvn clean package -DskipTests

# 运行阶段：用轻量级 JDK 镜像运行 jar 包
FROM openjdk:17-jdk-slim
WORKDIR /app

# 复制上一步生成的 jar 包
COPY --from=build /app/target/*.jar app.jar

# 暴露服务端口（和你的 Java 服务监听端口一致，比如 8080）
EXPOSE 8080

# 启动命令
ENTRYPOINT ["java", "-jar", "app.jar"]
