package TOOL.Calibrate;

import java.util.Vector;

import TOOL.TOOL;
import TOOL.Image.TOOLImage;
import TOOL.Image.ProcessedImage;
import TOOL.Image.ColorTable;
import TOOL.Data.*;

public class VisionState {
    
    private TOOLImage rawImage;
    private ProcessedImage thresholdedImage;
    private ColorTable  colorTable;

    private Vector<FieldObject> objects;
    private Ball ball;

    public VisionState(Frame f, ColorTable c) {
        rawImage = f.image();
        colorTable = c;

        if (rawImage != null && colorTable != null)
            thresholdedImage = new ProcessedImage(rawImage, colorTable);

        objects = new Vector<FieldObject>();
        ball = null;
    }

    public VisionState() {
        rawImage = null;
        thresholdedImage = null;
	colorTable = null;
	
        objects = new Vector<FieldObject>();
        ball = null;
    }

    public VisionState(TOOLImage newRawImage, 
		       ProcessedImage newThreshImage, 
		       ColorTable newColorTable){
	rawImage = newRawImage;
        thresholdedImage = newThreshImage;
	colorTable = newColorTable;
	
        objects = new Vector<FieldObject>();
        ball = null;
    }

    public TOOLImage getImage() {
        return rawImage;
    }

    public ProcessedImage getThreshImage() {
        return thresholdedImage;
    }

    public ColorTable getColorTable() {
        return colorTable;
    }


    public void setImage(TOOLImage i) {
        rawImage = i;
    }

    public void setThreshImage(ProcessedImage i) {
        thresholdedImage = i;
    }

    public void setColorTable(ColorTable c) {
        colorTable = c;
    }



    public void addObject(FieldObject obj) {
        objects.add(obj);
    }

    public void removeObject(FieldObject obj) {
        objects.remove(obj);
    }

    public void setBall(Ball b) {
        ball = b;
    }

    public Ball getBall() {
        return ball;
    }

    public void printWidth() {
        TOOL.CONSOLE.message("Image width: " + rawImage.getWidth());
    }
}
