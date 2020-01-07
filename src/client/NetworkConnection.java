package client;

import java.io.*;
import java.net.Socket;
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
            try (Socket socket = new Socket(getIP(), getPort());
                 BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                 PrintWriter out = new PrintWriter(socket.getOutputStream())) {

                this.socket = socket;
                this.out = out;

                socket.setTcpNoDelay(true);
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
