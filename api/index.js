import com.sun.net.httpserver.*;
import com.google.gson.*;

import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class VoteServer {
    static class Vote {
        String id;
        String topic;
        List<String> options;
        List<Integer> counts;

        public Vote(String id, String topic, List<String> options) {
            this.id = id;
            this.topic = topic;
            this.options = options;
            this.counts = new ArrayList<>(Collections.nCopies(options.size(), 0));
        }
    }

    private static final Map<String, Vote> votes = new HashMap<>();
    private static final Gson gson = new Gson();

    public static void main(String[] args) throws IOException {
        int port = Integer.parseInt(System.getenv().getOrDefault("PORT", "8080"));
        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);

        // 跨域配置
        server.createContext("/", exchange -> {
            exchange.getResponseHeaders().add("Access-Control-Allow-Origin", "*");
            exchange.getResponseHeaders().add("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            exchange.getResponseHeaders().add("Access-Control-Allow-Headers", "Content-Type");

            if (exchange.getRequestMethod().equals("OPTIONS")) {
                exchange.sendResponseHeaders(200, -1);
                return;
            }

            String path = exchange.getRequestURI().getPath();
            String response = "";
            int status = 200;

            if ("/list".equals(path)) {
                response = gson.toJson(votes.values());
            } else if ("/create".equals(path) && "POST".equals(exchange.getRequestMethod())) {
                InputStream is = exchange.getRequestBody();
                String body = new String(is.readAllBytes(), StandardCharsets.UTF_8);
                JsonObject obj = gson.fromJson(body, JsonObject.class);
                String topic = obj.get("topic").getAsString();
                List<String> options = gson.fromJson(obj.getAsJsonArray("options"), new com.google.gson.reflect.TypeToken<List<String>>() {}.getType());

                String id = UUID.randomUUID().toString();
                votes.put(id, new Vote(id, topic, options));
                response = gson.toJson(Map.of("success", true, "id", id));
            } else if ("/vote".equals(path) && "POST".equals(exchange.getRequestMethod())) {
                InputStream is = exchange.getRequestBody();
                String body = new String(is.readAllBytes(), StandardCharsets.UTF_8);
                JsonObject obj = gson.fromJson(body, JsonObject.class);
                String voteId = obj.get("voteId").getAsString();
                int optIndex = obj.get("optIndex").getAsInt();

                Vote vote = votes.get(voteId);
                if (vote != null && optIndex >= 0 && optIndex < vote.counts.size()) {
                    vote.counts.set(optIndex, vote.counts.get(optIndex) + 1);
                    response = gson.toJson(Map.of("success", true));
                } else {
                    status = 404;
                    response = gson.toJson(Map.of("error", "Invalid vote or option index"));
                }
            } else {
                status = 404;
                response = gson.toJson(Map.of("error", "Not found"));
            }

            exchange.sendResponseHeaders(status, response.getBytes(StandardCharsets.UTF_8).length);
            try (OutputStream os = exchange.getResponseBody()) {
                os.write(response.getBytes(StandardCharsets.UTF_8));
            }
        });

        server.start();
        System.out.println("Server started on port " + port);
    }
}
