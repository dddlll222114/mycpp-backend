FROM openjdk:17-jdk-slim
WORKDIR /app
COPY . .
RUN javac VoteServer.java && jar cvfe app.jar VoteServer *.class
EXPOSE 8080
CMD ["java", "-jar", "app.jar"]
