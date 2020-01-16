package client;

import javafx.beans.value.ChangeListener;
import javafx.beans.value.ObservableValue;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextArea;
import javafx.scene.control.TextField;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.text.TextAlignment;

import java.net.URL;
import java.util.ArrayList;
import java.util.ResourceBundle;

public class Controller implements Initializable {
    @FXML
    private Label phraseLbl;
    @FXML
    private ImageView imageView;
    @FXML
    private Label msgLbl;
    @FXML
    private Button sendBtn;
    @FXML
    private Button readyBtn;
    @FXML
    private TextField inputEdit;
    @FXML
    private TextArea scoreBoard;


    @Override
    public void initialize(URL url, ResourceBundle resourceBundle) {
        msgLbl.setWrapText(true);
        msgLbl.setTextAlignment(TextAlignment.CENTER);
        sendBtn.setDisable(false);
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

    public String getInputEditText() {
        return inputEdit.getText();
    }

    public void clearInputEdit() {
        this.inputEdit.clear();
    }

    public String getPhraseLblText() {
        return phraseLbl.getText();
    }

    public void setPhraseLblText(String phraseLbl) {
        this.phraseLbl.setText(phraseLbl);
    }

    public void setImageView(ImageView imageView) {
        this.imageView = imageView;
    }

    public void setMessageText(String msg) {
        this.msgLbl.setText(msg);
    }

    public void setScoreBoardText(String scoreBoard) {
        this.scoreBoard.setText(scoreBoard);
    }

    public Label getPhraseLbl() {
        return phraseLbl;
    }

    public void setPhraseLbl(Label phraseLbl) {
        this.phraseLbl = phraseLbl;
    }

    public ImageView getImageView() {
        return imageView;
    }

    public Label getMsgLbl() {
        return msgLbl;
    }

    public void setMsgLbl(Label msgLbl) {
        this.msgLbl = msgLbl;
    }

    public Button getSendBtn() {
        return sendBtn;
    }

    public void setSendBtn(Button sendBtn) {
        this.sendBtn = sendBtn;
    }

    public Button getReadyBtn() {
        return readyBtn;
    }

    public void setReadyBtn(Button readyBtn) {
        this.readyBtn = readyBtn;
    }

    public TextField getInputEdit() {
        return inputEdit;
    }

    public void setInputEdit(TextField inputEdit) {
        this.inputEdit = inputEdit;
    }

    public TextArea getScoreBoard() {
        return scoreBoard;
    }

    public void setScoreBoard(TextArea scoreBoard) {
        this.scoreBoard = scoreBoard;
    }

    public void disableAll() {
        sendBtn.setDisable(true);
        readyBtn.setDisable(true);
        inputEdit.setDisable(true);
    }

    public void drawScoreBoard(ArrayList<String> list) {
        for (var player : list) {
            String[] pom = player.split(" ");
            scoreBoard.appendText(pom[0] + "\t" + pom[1] + "\n");
        }
    }

    public void drawImage(int fails) {
        Image image;
        switch(fails) {
            case 0:
                image = new Image("resources/image/0.png");
                imageView.setImage(image);
                break;
            case 1:
                image = new Image("resources/image/1.png");
                imageView.setImage(image);
                break;
            case 2:
                image = new Image("resources/image/2.png");
                imageView.setImage(image);
                break;
            case 3:
                image = new Image("resources/image/3.png");
                imageView.setImage(image);
                break;
            case 4:
                image = new Image("resources/image/4.png");
                imageView.setImage(image);
                break;
            case 5:
                image = new Image("resources/image/5.png");
                imageView.setImage(image);
                break;
            case 6:
                image = new Image("resources/image/6.png");
                imageView.setImage(image);
                break;
            case 7:
                image = new Image("resources/image/7.png");
                imageView.setImage(image);
                break;
            default:
                imageView.setImage(null);
                break;
        }
    }
}
