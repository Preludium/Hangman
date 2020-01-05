package client;

import javafx.beans.value.ChangeListener;
import javafx.beans.value.ObservableValue;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextArea;
import javafx.scene.control.TextField;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;

import java.net.URL;
import java.util.ResourceBundle;

public class Controller implements Initializable {
    @FXML
    private Label phraseLbl;
    @FXML
    private ImageView imageView;
    @FXML
    private Label msgLbl;
    @FXML
    public Button sendBtn;
    @FXML
    private Button readyBtn;
    @FXML
    public TextField inputEdit;
    @FXML
    private TextArea scoreBoard;

    @Override
    public void initialize(URL url, ResourceBundle resourceBundle) {
        sendBtn.setDisable(true);
        // cant display image...
//        imageView = new ImageView(new Image(getClass().getResourceAsStream("/resources/image/start_img.jpg")));
        inputEdit.textProperty().addListener(new ChangeListener<String>() {
            @Override
            public void changed(ObservableValue<? extends String> observableValue, String oldValue, String newValue) {
                if(newValue.length() == 0)
                    sendBtn.setDisable(true);
                else
                    sendBtn.setDisable(false);
            }
        });
    }

    public String getInputEdit() {
        return inputEdit.getText();
    }

    public void clearInputEdit() {
        this.inputEdit.clear();
    }

    public void setMessageText(String msg) {
        this.msgLbl.setText(msg);
    }

    public void setScoreBoard(String scoreBoard) {
        this.scoreBoard.setText(scoreBoard);
    }
}
