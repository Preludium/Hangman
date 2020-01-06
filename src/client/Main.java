package client;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.stage.Stage;

public class Main extends Application {
    private final String ACCEPT = "ACCEPT";
    private final String NICK = "NICK";
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
    private String[] scoreBoard;

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
        connection = new Client("127.0.0.1", 8080, data -> {
//        Platform.runLater: If you need to update a GUI component from a non-GUI thread, you can use that to put
//        your update in a queue and it will be handled by the GUI thread as soon as possible.
            Platform.runLater(() -> {
                if (data != null) {
                    controller.setScoreBoard(data);
                    if (data.equals("SERVER DOWN")) {
                        controller.setMessageText("Server is down. Come back later...");
                        controller.disableAll();
                    } else if (data.equals("SERVER CLOSED")) {
                        controller.setMessageText("Server closed connection");
                        controller.disableAll();
                        handleCloseConn();
                    } else if (data.contains(ACCEPT)) {
                        controller.setMessageText("Connected to server");
                    } else if (data.contains(KICK)) {
                        controller.setMessageText("You have been kicked from the server. Restart application to reconnect");
                        controller.disableAll();
                        handleCloseConn();
                    } else if (data.contains(GAME)) {
                        controller.setMessageText("Starting game");
                        int num = Integer.parseInt(data.substring(6));
                        phrase = "*".repeat(num);
                        controller.setPhraseLbl(phrase);
                        controller.setMessageText("Choose letter");
                    } else if (data.contains(REFUSE)) {
                        controller.setMessageText("Server refused your connection request. Restart application to reconnect");
                        controller.disableAll();
                    } else if (data.contains(COUNT)) {
                        String time = data.substring(9);
                        controller.setMessageText("Game will start in " + time + " s");
                    } else if (data.contains(OVER)) {
                        controller.setMessageText("Game ended");
                        controller.disableAll();
                    } else if (data.contains(WAIT)) {
                        controller.setMessageText("Waiting for min 2 clients to start countdown");
                        controller.disableAll();
                    } else if (data.contains(GOOD)) {
                        controller.setMessageText(data);
//                        controller.setMessageText("Successful guess");
//                        char letter = data.charAt(5);
//                        String[] positions = data.substring(7).split(" ");
                    } else if (data.contains(BAD)) {
                        controller.setMessageText(data);
//                        brak obslugi
//                        controller.setMessageText("Fail");
//                        String fails = data.substring(4);
//                        updateImage(fails);
                    }
                }
            });
        });
    }

    public void setUp() {
        controller.setMessageText("Type your nick");
        controller.sendBtn.setText("Confirm");

        controller.sendBtn.setOnAction(new EventHandler<ActionEvent>() {
            @Override
            public void handle(ActionEvent actionEvent) {
//                connection.send(controller.getInputEdit());
                nick = controller.getInputEdit();
                controller.clearInputEdit();
                nickChosen();
            }
        });

        controller.inputEdit.setOnKeyPressed(new EventHandler<KeyEvent>() {
            @Override
            public void handle(KeyEvent ke) {
                if (ke.getCode().equals(KeyCode.ENTER)) {
                    if (!controller.getInputEdit().isEmpty()) {
//                        connection.send(controller.getInputEdit());
                        nick = controller.getInputEdit();
                        controller.clearInputEdit();
                        nickChosen();
                    }
                }
            }
        });
    }

    public void nickChosen() {

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

        connection.startConnection();
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
