
// This file is part of TOOL, a robotics interaction and development
// package created by the Northern Bites RoboCup team of Bowdoin College
// in Brunswick, Maine.
//
// TOOL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TOOL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TOOL.  If not, see <http://www.gnu.org/licenses/>.


/**
 * This class provides the link to cpp. To get the results of vision processing
 * from cpp, you can call visionLink.processImage(), which returns a
 * thresholded image.
 *
 *
 * Future Work:
 * IMPORTANT: When the cpp sublayer throws an error, we cannot catch it.
 *            The TOOL will simply crash. For exaple, if cpp tries to load
 *            A table from a path, and fails, the TOOL crashes.
 *
 * It is not clear what the right format for the img_data is.
 * Vision expects to get a pointer to a continuous 1D array of length w*h*2
 * Java
 *
 * In the future, we'd like to get back much more information --
 * like field objects, etc
 */

package edu.bowdoin.robocup.TOOL.Vision;

import edu.bowdoin.robocup.TOOL.TOOL;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.lang.UnsatisfiedLinkError;

public class TOOLVisionLink {
    //These are defined here for ease, but need to get read from somewhere else
    public static final  int DFLT_IMAGE_WIDTH = 640;
    public static final  int DFLT_IMAGE_HEIGHT = 480;

    int width;
    int height;

    TOOL tool;
    static private boolean visionLinkSuccessful;

    public TOOLVisionLink(TOOL _t) {
        setImageDimensions(DFLT_IMAGE_WIDTH, DFLT_IMAGE_HEIGHT);
        tool = _t;
    }

    /**
     * Method sets the size of image, etc that we are sending to cpp
     * Important: The cpp lower level will rejects images with the wrong
     * dimensions
     */
    public void setImageDimensions(int w, int h)
    {
        width = w; height = h;
    }
    public boolean isLinkActive()
    {
        return visionLinkSuccessful;
    }
    public byte[][] processImage(byte[] img_data, float[] joint_data,
                                 String colorTablePath )
    {
        byte[][] threshResult = new byte[height][width];
        if( visionLinkSuccessful){
            try{
                cppProcessImage(img_data,joint_data,
                                colorTablePath,threshResult);
            }catch(Throwable e){
                tool.CONSOLE.error("Error in cpp sub system. \n"+
                                   "   Processing failed.");
            }
        }
        else
            tool.CONSOLE.message("VisionLink inactive,"+
                                 " so image processing failed");
        return threshResult;

    }

    //Native methods:
    native private void cppProcessImage(byte[] img_data, float[] joint_data,
                                        String colorTablePath,
                                        byte[][] threshResult);

    //Load the cpp library that implements the native methods
    static
    {
        try{
            System.loadLibrary("TOOLVisionLink");
            visionLinkSuccessful = true;
        }catch(UnsatisfiedLinkError e){
            visionLinkSuccessful = false;
            System.err.print("Vision Link Failed to initialize: ");
            System.err.println(e.getMessage());
        }

    }


}