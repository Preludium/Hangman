package pl.hangman;

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
    public Label phraseLbl;
    public ImageView imageView;
    public Label msgLbl;
    public Button sendBtn;
    public Button readyBtn;
    public TextField inputEdit;
    public TextArea scoreBoard;

    @Override
    public void initialize(URL url, ResourceBundle resourceBundle) {
        // cant display image...
        imageView = new ImageView(new Image(getClass().getResourceAsStream("/resources/image/start_img.jpg")));
    }
}
