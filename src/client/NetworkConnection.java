package client;

import java.io.*;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

public abstract class NetworkConnection {
    private ConnectionThread connThread;
    private Consumer<String> onReceiveCallback;

    public NetworkConnection(Consumer<String> onReceiveCallback) {
        this.onReceiveCallback = onReceiveCallback;
        this.connThread = new ConnectionThread();
        this.connThread.setDaemon(true);
    }

    public Socket getSocket() {
        return connThread.socket;
    }

    public void startConnection() {
        connThread.start();
    }

    public void send(String data) {
        connThread.out.print(data);
        connThread.out.flush();
    }

    public void closeConnection() throws Exception {
        Main.socketCreatedProperty.setValue(false);
        if(connThread.socket != null)
            connThread.socket.close();
    }

    protected abstract String getIP();
    protected abstract int getPort();

    private class ConnectionThread extends Thread {

        private Socket socket;
        private PrintWriter out;

        @Override
        public void run() {
            Socket socket = null;
            for(int i = 0; i < 6; i++) {
                try {
                    socket = new Socket(getIP(), getPort());
                    if (socket != null) {
                        break;
                    }
                    TimeUnit.MILLISECONDS.sleep(500);
                } catch (IOException | InterruptedException e) {
                    onReceiveCallback.accept("SERVER DOWN");
                    return;
                }
            }

            try (BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                 PrintWriter out = new PrintWriter(socket.getOutputStream())) {

                this.out = out;
                this.socket = socket;
                this.socket.setTcpNoDelay(true);
                onReceiveCallback.accept("SOCKET CREATED");

                while(true) {
                    String line = in.readLine();
                    if (line == null) {
                        onReceiveCallback.accept("SERVER CLOSED");
                        break;
                    }
//                    System.out.println(line);
                    onReceiveCallback.accept(line);
                }
            } catch (Exception e) {
                if(socket == null)
                    onReceiveCallback.accept("SERVER DOWN");
            }
        }
    }
}
