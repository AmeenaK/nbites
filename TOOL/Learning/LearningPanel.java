package TOOL.Learning;

import javax.swing.JPanel;
import javax.swing.JCheckBox;
import javax.swing.JTextArea;
import java.awt.Component;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.event.ChangeListener;
import javax.swing.event.ChangeEvent;
import javax.swing.JComponent;
import javax.swing.JSlider;
import javax.swing.JButton;
import java.awt.event.KeyListener;
import java.awt.event.KeyEvent;

import java.awt.event.MouseWheelListener;
import java.awt.event.MouseWheelEvent;


import javax.swing.InputMap;
import javax.swing.ActionMap;
import java.awt.event.InputEvent;
import javax.swing.KeyStroke;
import javax.swing.AbstractAction;
import TOOL.Data.DataListener;
import TOOL.Data.Frame;
import TOOL.Data.DataSet;
import javax.swing.BoxLayout;
import javax.swing.JTextPane;
import javax.swing.JTextField;
import javax.swing.JComboBox;
import javax.swing.text.*;
import java.awt.GridLayout;
import java.awt.Font;
import java.awt.Dimension;
import java.awt.Cursor;

import TOOL.Image.ImageOverlay;

import TOOL.Calibrate.ColorSwatchParent;
import TOOL.TOOL;


public class LearningPanel extends JPanel implements DataListener, KeyListener {

    private JCheckBox human, ball;
    private JTextField jumpToFrame;
	private JButton prevImage, nextImage, jumpToButton;
    private JTextPane feedback;
    private InputMap im;
    private ActionMap am;
    protected JCheckBox drawColors;

    private Learning learn;

    private static final int NUM_COLUMNS = 20;
    private static final int NUM_ROWS = 2;

    public LearningPanel(Learning aLearn) {
        super();
		learn = aLearn;
        setupWindow();
        setupListeners();
    }

    private void setupWindow() {

        // centering text from http://forum.java.sun.com/thread.jspa?threadID=166685&messageID=504493
        feedback = new JTextPane();
        // Make the text centered
        SimpleAttributeSet set = new SimpleAttributeSet();
        StyledDocument doc = feedback.getStyledDocument();
        StyleConstants.setAlignment(set,StyleConstants.ALIGN_CENTER);
        feedback.setParagraphAttributes(set,true);

        feedback.setEditable(false);
        feedback.setText("Welcome to TOOL 1.0");
        // Make the background match in color
        feedback.setBackground(this.getBackground());

        prevImage = new JButton("Previous (S)");
        prevImage.setFocusable(false);

        nextImage = new JButton("Next (D)");
        nextImage.setFocusable(false);

        jumpToFrame = new JTextField("0", 4);

        jumpToButton = new JButton("Jump");
        jumpToButton.setFocusable(false);

        setLayout(new BoxLayout(this, BoxLayout.LINE_AXIS));


        JPanel navigation = new JPanel();
        navigation.setLayout(new GridLayout(4,2));
        navigation.add(prevImage);
        navigation.add(nextImage);
        navigation.add(jumpToFrame);
        navigation.add(jumpToButton);

        // Size the navigation panel to only take up as much room as needed
        Dimension navigationSize = new Dimension(2 * (int) prevImage.getPreferredSize().getWidth(), 4 * (int) jumpToFrame.getPreferredSize().getWidth());
        navigation.setMinimumSize(navigationSize);
        navigation.setPreferredSize(navigationSize);
        navigation.setMaximumSize(navigationSize);
        add(navigation);
    }


    private void setupListeners() {
        learn.getTool().getDataManager().addDataListener(this);

        prevImage.addActionListener(new ActionListener(){
                public void actionPerformed(ActionEvent e) {
                    learn.getLastImage();
                }
            });

        nextImage.addActionListener(new ActionListener(){
                public void actionPerformed(ActionEvent e) {
                    learn.getNextImage();
                }
            });

        jumpToFrame.addKeyListener(new KeyListener(){
                public void keyPressed(KeyEvent e){
                    if (e.getKeyCode() == KeyEvent.VK_ESCAPE) {
                        jumpToFrame.transferFocus();
                    } else if (e.getKeyCode() == KeyEvent.VK_ENTER) {
                        jumpToButton.doClick();
                    }
                }
                public void keyTyped(KeyEvent e) {}
                public void keyReleased(KeyEvent e){}
            });

        jumpToButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    try {
                        int newIndex = Integer.parseInt(jumpToFrame.getText());
                        learn.setImage(newIndex);
                        jumpToFrame.transferFocus();
                    }
                    // If they didn't have a legit number in the field, this
                    // catches the problem to avoid a crash.
                    catch (NumberFormatException f) {
                        jumpToFrame.setText("Invalid");
                        jumpToFrame.setSelectionStart(0);
                        jumpToFrame.setSelectionEnd("Invalid".length());
                        return;
                    }
                }
            });

    }

    /**
     * Greys out buttons depending on whether we can actually use them at this
     * moment; e.g. undo button is initially grey because you cannot undo until
     * there is something on the undo stack.  Similarly for the previous image
     * and next image buttons.  Finally, fill holes and the jump button and
     * text area are only accessible if we have an image loaded.
     */
    public void fixButtons() {
        prevImage.setEnabled(learn.canGoBackward());
        nextImage.setEnabled(learn.canGoForward());
        jumpToFrame.setEnabled(learn.hasImage());
        jumpToButton.setEnabled(learn.hasImage());
    }

    public void setText(String text) {
        feedback.setText(text);
    }

    public String getText() {
	return feedback.getText();
    }

	public void setOverlays() {
	}


    public void notifyDataSet(DataSet s, Frame f) {

    }

    /** Set the text in the box to update the frame number. */
    public void notifyFrame(Frame f) {
        jumpToFrame.setText((new Integer(f.index())).toString());
    }

    public void keyPressed(KeyEvent e) {}
    public void keyReleased(KeyEvent e) {}
    public void keyTyped(KeyEvent e) {}

}

