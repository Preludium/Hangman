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

        Socket socket;
        PrintWriter out;

        @Override
        public void run() {
            try (Socket socket = new Socket(getIP(), getPort());
                 BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                 PrintWriter out = new PrintWriter(socket.getOutputStream())) {

                this.socket = socket;
                this.out = out;

                socket.setTcpNoDelay(true);
                while(true) {
//                    if (!socket.getInetAddress().isReachable(5))
//                        onReceiveCallback.accept("SERVER CLOSED");

                    String line = in.readLine();
                    onReceiveCallback.accept(line);
                }
            } catch (Exception e) {
                onReceiveCallback.accept("SERVER DOWN");
//                e.printStackTrace();
            }
        }
    }
}
