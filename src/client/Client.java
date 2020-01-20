package client;

import java.util.function.Consumer;

public class Client extends NetworkConnection {

    private String ip;
    private int port;

    public Client (String ip, int port, Consumer<String> onReceiveCallback) {
        super(onReceiveCallback);
        this.ip = ip;
        this.port = port;
    }

    @Override
    protected String getIP() {
        return this.ip;
    }

    @Override
    protected int getPort() {
        return this.port;
    }
}
