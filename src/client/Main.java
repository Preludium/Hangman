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

import java.util.ArrayList;
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

    private Controller controller;
    private NetworkConnection connection = new Client("127.0.0.1", 8080, data -> {
//        Platform.runLater: If you need to update a GUI component from a non-GUI thread, you can use that to put
//        your update in a queue and it will be handled by the GUI thread as soon as possible.
        Platform.runLater(() -> {
            if (data != null) {
//                controller.setScoreBoard(data);
                if (data.equals("SERVER DOWN")) {
                    controller.setMessageText("Server is down. Come back later...");
                    controller.disableAll();
                } else if (data.equals("SERVER CLOSED")) {
                    controller.setMessageText("Server closed connection");
                    controller.disableAll();
                    handleCloseConn();
                } else if (data.contains(ACCEPT)) {
                    controller.setMessageText("Connected to server");
                } else if (data.contains(REFUSE)) {
                    controller.setMessageText("This nick is taken. Restart application to choose another one");
                    controller.disableAll();
                    handleCloseConn();
                } else if (data.contains(KICK)) {
                    controller.setMessageText("You have been kicked from the server. Restart application to reconnect");
                    controller.disableAll();
                    handleCloseConn();
                } else if (data.contains(GAME)) {
                    controller.setMessageText("Starting game");
                    int num = Integer.parseInt(data.substring(5));
                    phrase = "*".repeat(num);
                    controller.setPhraseLbl(phrase);
                    controller.setMessageText("Choose letter");
                    controller.setScoreBoard("");
                } else if (data.contains(COUNT)) {
                    String time = data.substring(6);
                    controller.setMessageText("Game will start in " + time + " s");
                } else if (data.contains(OVER)) {
                    controller.setMessageText("Game ended");
                    players = Integer.parseInt(data.substring(5));
//                    System.out.println(players);
                    scoreBoard = new ArrayList<>();
                    controller.disableAll();
                } else if (data.contains(PLAYER)) {
                    scoreBoard.add(data.substring(7));
//                    System.out.println(data.substring(7));
                    if (scoreBoard.size() == players) {
                        controller.drawScoreBoard(scoreBoard);
                        players = 0;
                        scoreBoard = null;
                    }
                } else if (data.contains(WAIT)) {
                    controller.setMessageText("Waiting for min 2 clients to start countdown. Click ready to join");
                    controller.disableAll();
                    controller.readyBtn.setDisable(false);
                } else if (data.contains(GOOD)) {
                    controller.setMessageText("Successful guess");
                    char letter = data.charAt(5);
                    String[] positions = data.substring(7).split(" ");
                    for (var pos : positions) {
                        int x = Integer.parseInt(pos);
                        phrase = phrase.substring(0, x) + letter + phrase.substring(x + 1);
                    }
                    controller.setPhraseLbl(phrase);
                } else if (data.contains(BAD)) {
                    int fails = Integer.parseInt(data.substring(4));
                    controller.setMessageText("Fail, " + data.substring(4) + " chances left");
                    controller.drawImage(fails);
                }
            }
        });
    });

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

    public void sendNick(){
        connection.send("NICK " + nick);
    }

    public void setUp() {
        controller.setMessageText("Type your nick");
        controller.sendBtn.setText("Confirm");

        controller.sendBtn.setOnAction(new EventHandler<ActionEvent>() {
            @Override
            public void handle(ActionEvent actionEvent) {
                nick = controller.getInputEdit();
                controller.clearInputEdit();
                setUpAfterNick();
            }
        });

        controller.inputEdit.setOnKeyPressed(new EventHandler<KeyEvent>() {
            @Override
            public void handle(KeyEvent ke) {
                if (ke.getCode().equals(KeyCode.ENTER)) {
                    if (!controller.getInputEdit().isEmpty()) {
                        nick = controller.getInputEdit();
                        controller.clearInputEdit();
                        setUpAfterNick();
                    }
                }
            }
        });
    }

    public void setUpAfterNick() {
        connection.startConnection();
        try {
            TimeUnit.SECONDS.sleep(1);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        if(connection.getSocket() != null)
            sendNick();

        controller.setMessageText("");
        controller.sendBtn.setText("Send");

        controller.sendBtn.setOnAction(new EventHandler<ActionEvent>() {
            @Override
            public void handle(ActionEvent actionEvent) {
                connection.send(controller.getInputEdit());
                controller.clearInputEdit();
            }
        });

        controller.readyBtn.setOnAction(new EventHandler<ActionEvent>() {
            @Override
            public void handle(ActionEvent actionEvent) {
                connection.send(READY);
                controller.readyBtn.setDisable(true);
            }
        });

        controller.inputEdit.setOnKeyPressed(new EventHandler<KeyEvent>()
        {
            @Override
            public void handle(KeyEvent ke)
            {
                if (ke.getCode().equals(KeyCode.ENTER))
                {
                    if(!controller.getInputEdit().isEmpty()) {
                        connection.send(controller.getInputEdit());
                        controller.clearInputEdit();
                    }
                }
            }
        });

        controller.inputEdit.setTextFormatter(new TextFormatter<String>((TextFormatter.Change change) -> {
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
