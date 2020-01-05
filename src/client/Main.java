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
    private final String accept = "ACCEPT";
    private final String kick = "KICK";

    private Controller controller;
    private NetworkConnection connection = new Client("127.0.0.1", 8080, data -> {
//        Platform.runLater: If you need to update a GUI component from a non-GUI thread, you can use that to put
//        your update in a queue and it will be handled by the GUI thread as soon as possible.
        Platform.runLater(() -> {
            if (data.equals("SERVER DOWN"))
                controller.setMessageText("Server is down. Come back later...");
            else if (data.equals("SERVER CLOSED"))
                controller.setMessageText("Server closed connection");
            else if (data.equals(accept))
                controller.setMessageText("Connected to server");
            else if (data.equals(kick))
                controller.setMessageText("You have benn kicked from the server. Restart application to reconnect");
            else
                controller.setScoreBoard(data);
        });
    });

    @Override
    public void init() {
        connection.startConnection();
    }

    @Override
    public void start(Stage primaryStage) throws Exception{
        FXMLLoader loader = new FXMLLoader(getClass().getResource("/resources/fxml/main.fxml"));
        Parent root = loader.load();
        controller = loader.getController();
        controller.sendBtn.setOnAction(new EventHandler<ActionEvent>() {
            @Override
            public void handle(ActionEvent actionEvent) {
                connection.send(controller.getInputEdit());
                controller.clearInputEdit();
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
        primaryStage.setTitle("Hangman");
        primaryStage.setScene(new Scene(root, 700, 400));
        primaryStage.setResizable(false);
        primaryStage.show();
    }

    @Override
    public void stop() throws Exception {
        connection.closeConnection();
    }

    public static void main(String[] args) {
        launch(args);
    }
}
