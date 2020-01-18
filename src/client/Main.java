package client;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.beans.property.BooleanProperty;
import javafx.beans.property.SimpleBooleanProperty;
import javafx.beans.value.ChangeListener;
import javafx.beans.value.ObservableValue;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.TextFormatter;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.stage.Stage;

import java.io.*;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;

public class Main extends Application {
    private final String ACCEPT = "ACCEPT";
    private final String PLAYER = "PLAYER";
    private final String REFUSE = "REFUSE";
    private final String KICK = "KICK";
    private final String GAME = "GAME";
    private final String COUNT = "COUNT";
    private final String OVER = "OVER";
    private final String WAIT = "WAIT";
    private final String READY = "READY";
    private final String GOOD = "GOOD";
    private final String BAD = "BAD";

    private String phrase;
    private String nick;
    private ArrayList<String> scoreBoard;
    private int players;
    private String adress;
    private int port;

    private Controller controller;
    private NetworkConnection connection;

    @Override
    public void start(Stage primaryStage) throws Exception{
        FXMLLoader loader = new FXMLLoader(getClass().getResource("/resources/fxml/main.fxml"));
        Parent root = loader.load();
        controller = loader.getController();
        primaryStage.setTitle("Hangman");
        primaryStage.setScene(new Scene(root, 700, 400));
        primaryStage.setResizable(false);
        primaryStage.show();
        setUp();
    }

    public void readServerAdress() {
        URL path = Main.class.getResource("serverIp.txt");
        try(BufferedReader br = new BufferedReader(new FileReader(new File(path.getFile())))) {
            String[] list = br.readLine().split(":");
            adress = list[0];
            port = Integer.parseInt(list[1]);
        } catch (IOException ex) {
            controller.getMsgLbl().setText("serverIp.txt io error");
            controller.disableAll();
        } catch (Exception ex) {
            controller.getMsgLbl().setText("something is wrong with server adress in serverIp.txt file");
            controller.disableAll();
        }
    }

    public void sendNick(){
        connection.send("NICK " + nick);
    }

    public void reconnect() {
        controller.getReadyBtn().setDisable(false);
        controller.getReadyBtn().setText("Reconnect");
        controller.getReadyBtn().setOnAction(new EventHandler<ActionEvent>() {
            @Override
            public void handle(ActionEvent actionEvent) {
                controller.getInputEdit().setDisable(false);
                controller.getSendBtn().setDisable(false);
                setUp();
                controller.getReadyBtn().setDisable(true);
                controller.getReadyBtn().setText("Ready");
            }
        });
    }

    public void setUp() {
        readServerAdress();
        connection = new Client(adress, port, data -> {
//        Platform.runLater: If you need to update a GUI component from a non-GUI thread, you can use that to put
//        your update in a queue and it will be handled by the GUI thread as soon as possible.
            Platform.runLater(() -> {
                if (data != null) {
                    System.out.println(data);
                    String[] splt = data.split(" ");
                    if (data.equals("SERVER DOWN")) {
                        controller.setMessageText("Server is down. Reconnect or come back later...");
                        controller.disableAll();
                        reconnect();
                    } else if (data.equals("SERVER CLOSED")) {
                        controller.setMessageText("Server closed connection");
                        controller.clearInputEdit();
                        controller.disableAll();
                        controller.getPhraseLbl().setText("");
                        handleCloseConn();
                        reconnect();
                    } else if (splt[0].contains(ACCEPT)) {
                        controller.setMessageText("Connected. Waiting for a new game session");
                    } else if (splt[0].contains(REFUSE)) {
                        controller.setMessageText("This nick is taken. Reconnect to choose new one");
                        controller.disableAll();
                        handleCloseConn();
                        reconnect();
                    } else if (splt[0].contains(KICK)) {
                        controller.setMessageText("You have been kicked from the server");
                        controller.getPhraseLbl().setText("");
                        controller.getImageView().setImage(null);
                        controller.getScoreBoard().setText("");
                        controller.disableAll();
                        handleCloseConn();
                        reconnect();
                    } else if (splt[0].contains(GAME)) {
                        controller.setMessageText("Starting game");
                        int num = Integer.parseInt(splt[1]);
                        phrase = "*".repeat(num);
                        controller.setPhraseLblText(phrase);
                        controller.setMessageText("Choose letter");
                        controller.setScoreBoardText("");
                        controller.getImageView().setImage(null);
                        controller.getSendBtn().setDisable(false);
                        controller.getInputEdit().setDisable(false);
                        controller.getScoreBoard().setText("");
                    } else if (splt[0].contains(COUNT)) {
                        String time = splt[1];
                        controller.setMessageText("Game will start in " + time + " s. Please confirm you are ready");
                        controller.disableAll();
                        controller.getReadyBtn().setDisable(false);
                    } else if (splt[0].contains(OVER)) {
                        controller.setMessageText("Game over");
                        players = Integer.parseInt(splt[1]);
                        scoreBoard = new ArrayList<>();
                        controller.disableAll();
                    } else if (splt[0].contains(PLAYER)) {
                        scoreBoard.add(splt[1] + " " + splt[2]);
                        if (scoreBoard.size() == players) {
                            controller.drawScoreBoard(scoreBoard);
                            players = 0;
                            scoreBoard = null;
                        }
                    } else if (splt[0].contains(WAIT)) {
                        controller.setMessageText("Not enough players. Waiting for at least two...");
                        controller.disableAll();
//                        controller.getReadyBtn().setDisable(false);
                    } else if (splt[0].contains(GOOD)) {
                        controller.setMessageText("Successful guess");
                        char letter = splt[1].charAt(0);
                        String[] positions = Arrays.copyOfRange(splt, 2, splt.length);
                        for (var pos : positions) {
                            int x = Integer.parseInt(pos);
                            phrase = phrase.substring(0, x) + letter + phrase.substring(x + 1);
                        }
                        controller.setPhraseLblText(phrase);
                    } else if (splt[0].contains(BAD)) {
                        int fails = Integer.parseInt(splt[1]);
                        if (fails > 0) {
                            controller.setMessageText("Fail, " + splt[1]+ " chances left");
                            controller.drawImage(fails);
                        } else {
                            controller.setMessageText("Game over. Waiting for scoreboard");
                            controller.getInputEdit().setDisable(true);
                            controller.getSendBtn().setDisable(true);
                        }
                    }
                }
            });
        });

        if (!controller.getInputEdit().isDisabled()) {
            controller.setMessageText("Type your nick");
            controller.getSendBtn().setText("Confirm");
        }

        controller.getSendBtn().setOnAction(new EventHandler<ActionEvent>() {
            @Override
            public void handle(ActionEvent actionEvent) {
                nick = controller.getInputEditText();
                controller.clearInputEdit();
                controller.setMessageText("Connecting to server...");
                setUpAfterNick();
            }
        });

        controller.getInputEdit().setOnKeyPressed(new EventHandler<KeyEvent>() {
            @Override
            public void handle(KeyEvent ke) {
                if (ke.getCode().equals(KeyCode.ENTER)) {
                    if (!controller.getInputEditText().isEmpty()) {
                        nick = controller.getInputEditText();
                        controller.clearInputEdit();
                        controller.setMessageText("Connecting to server...");
                        setUpAfterNick();
                    }
                }
            }
        });

        controller.getInputEdit().setTextFormatter(new TextFormatter<String>((TextFormatter.Change change) -> {
            String newText = change.getControlNewText();
            if (newText.length() > 15 || newText.contains(" ")) {
                return null ;
            } else {
                return change ;
            }
        }));
    }

    public void setUpAfterNick() {
        connection.startConnection();
        try {
            TimeUnit.MILLISECONDS.sleep(5);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        if(connection.getSocket() != null)
            sendNick();

        controller.getSendBtn().setText("Send");
        controller.getInputEdit().setDisable(true);

        controller.getSendBtn().setOnAction(new EventHandler<ActionEvent>() {
            @Override
            public void handle(ActionEvent actionEvent) {
                connection.send(controller.getInputEditText().toLowerCase());
                controller.clearInputEdit();
            }
        });

        controller.getReadyBtn().setOnAction(new EventHandler<ActionEvent>() {
            @Override
            public void handle(ActionEvent actionEvent) {
                connection.send(READY);
                controller.getReadyBtn().setDisable(true);
                controller.getMsgLbl().setText("Waiting for at least 2 clients to start game. You are ready");
            }
        });

        controller.getInputEdit().setOnKeyPressed(new EventHandler<KeyEvent>()
        {
            @Override
            public void handle(KeyEvent ke)
            {
                if (ke.getCode().equals(KeyCode.ENTER))
                {
                    if(!controller.getInputEditText().isEmpty()) {
                        connection.send(controller.getInputEditText().toLowerCase());
                        controller.clearInputEdit();
                    }
                }
            }
        });

        controller.getInputEdit().setTextFormatter(new TextFormatter<String>((TextFormatter.Change change) -> {
            String newText = change.getControlNewText();
            if (newText.length() > 1) {
                return null ;
            } else {
                return change ;
            }
        }));

    }

    public void handleCloseConn() {
        try {
            connection.closeConnection();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void stop() throws Exception {
        connection.closeConnection();
    }

    public static void main(String[] args) {
        launch(args);
    }
}
