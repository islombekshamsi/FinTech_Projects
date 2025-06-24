package org.example;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.XYChart;
import javafx.stage.Stage;
import yahoofinance.Stock;
import yahoofinance.YahooFinance;

import java.math.BigDecimal;
import java.time.LocalTime;
import java.time.format.DateTimeFormatter;
import java.util.Timer;
import java.util.TimerTask;

public class App extends Application {

    private final XYChart.Series<Number, Number> series = new XYChart.Series<>();
    private int timeCounter = 0;

    @Override
    public void start(Stage primaryStage) {
        primaryStage.setTitle("Dow Jones Live Stock Price Chart");

        NumberAxis xAxis = new NumberAxis();
        NumberAxis yAxis = new NumberAxis();
        xAxis.setLabel("Time (s)");
        yAxis.setLabel("Price (USD)");

        LineChart<Number, Number> lineChart = new LineChart<>(xAxis, yAxis);
        lineChart.setTitle("Dow Jones Price Over Time");
        lineChart.setAnimated(false);

        series.setName("Dow Jones (^DJI)");
        lineChart.getData().add(series);

        Scene scene = new Scene(lineChart, 800, 600);
        primaryStage.setScene(scene);
        primaryStage.show();

        startFetchingData();
    }

    private void startFetchingData() {
        Timer timer = new Timer();

        timer.scheduleAtFixedRate(new TimerTask() {
            public void run() {
                try {
                    Stock dowJones = YahooFinance.get("^DJI");
                    if (dowJones != null && dowJones.getQuote() != null) {
                        BigDecimal price = dowJones.getQuote().getPrice();
                        int currentTime = timeCounter++;

                        if (price != null) {
                            Platform.runLater(() -> {
                                series.getData().add(new XYChart.Data<>(currentTime, price));
                                if (series.getData().size() > 100) {
                                    series.getData().remove(0); // Keep chart clean
                                }
                            });

                            System.out.println(LocalTime.now().format(DateTimeFormatter.ofPattern("HH:mm:ss")) +
                                    " - Dow Jones: $" + price);
                        }
                    } else {
                        System.out.println("Failed to retrieve Dow Jones data.");
                    }
                } catch (Exception e) {
                    System.out.println("Error fetching stock data: " + e.getMessage());
                }
            }
        }, 0, 5000); // every 5 seconds
    }

    public static void main(String[] args) {
        launch(args);
    }
}