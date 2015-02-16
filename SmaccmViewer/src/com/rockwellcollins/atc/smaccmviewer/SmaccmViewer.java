package com.rockwellcollins.atc.smaccmviewer;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.image.BufferedImage;
import java.awt.image.WritableRaster;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.net.ConnectException;
import java.net.Socket;

import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.Timer;


public class SmaccmViewer extends JPanel implements Runnable, ActionListener{

	private static final long serialVersionUID = 8302649087754364851L;
	
//	private String serverName = "192.168.1.123";
//	private final String serverName = "localhost";
	private static String serverName;
	private final int port = 4000;
	
	private Image i = null;
	private static final int width = 320;
	private static final int height = 200;
	private Socket client;
	private DataInputStream in;
	private PrintWriter out;
	private final JFrame frame = new JFrame("SmaccmCopter Video");
	
	private Timer timer = new Timer(50, this);
	
	public static void main(String[] args) {
		
		if(args.length != 1){
			System.out.println("Usage: java SmaccmViewer.jar [hostname|ip address]");
			System.exit(0);
		}
		
		serverName = args[0];
		
		final SmaccmViewer viewer = new SmaccmViewer();
		viewer.init();
		Thread thread = new Thread(viewer);
		thread.start();

	}

	public void init() {

		try {
			client = new Socket(serverName, port);
			InputStream inFromServer = client.getInputStream();
			in = new DataInputStream(inFromServer);
			out = new PrintWriter(client.getOutputStream(), true);
		} catch(ConnectException e) {
			System.err.printf("Connection to %s:%d refused\n", serverName, port);
			System.exit(ABORT);
		} catch (IOException e) {
			e.printStackTrace();
			System.exit(ABORT);
		} 
		
		frame.setSize(width, height);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.setVisible(true);
		frame.add(this);
		timer.start();

	}

	public void run() {
		int[] pixels = new int[width*height*3];
		byte[] networkBytes = new byte[width*height*3];
		
		try {
			while(true){
				in.readFully(networkBytes);
				for(int i = 0; i < pixels.length; i++){
					pixels[i] = networkBytes[i];
				}

				BufferedImage image = new BufferedImage(width, height, BufferedImage.TYPE_3BYTE_BGR);
				WritableRaster raster = (WritableRaster) image.getData();
				raster.setPixels(0, 0, width, height, pixels);
				image.setData(raster);
				i = image;
				//on the server side we keep going if anything is received

                //these are acks that are received by the sender to let
			    //her know that we are still listening. after it receives
			    //a single byte of anything it proceeds to send the next frame
			    out.print('a');
			    out.flush();
			}
			
		} catch(EOFException e) {
			System.err.printf("Connection to %s:%d lost\n", serverName, port);
			System.exit(ABORT);

		}catch(IOException e){
			e.printStackTrace();
		}
	}

	@Override
	public void actionPerformed(ActionEvent evt) {
		repaint();
	}
	
	@Override
	public void paintComponent(Graphics g){
		if(i != null){
			g.drawImage(i, 0, 0, null);
		}
	}

}
