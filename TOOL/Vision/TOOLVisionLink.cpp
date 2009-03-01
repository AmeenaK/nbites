/**
 * This is the cpp end of the JNI connection between the tool and the cpp vision
 * algorithm. See the method processFrame() implemented below.
 *
 * @author Johannes Strom
 * @author Mark McGranagan
 *
 * @date November 2008
 */

#include <jni.h>

#include <iostream>
#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include "TOOL_Vision_TOOLVisionLink.h"
#include "Vision.h"
#include "NaoPose.h"
#include "Sensors.h"
#include "SensorDef.h"       // for NUM_SENSORS
#include "Kinematics.h"      // for NUM_JOINTS
#include "Structs.h"         // for estimate struct
using namespace std;
using namespace boost;

// static long long
// micro_time (void)
// {
// #ifdef __GNUC__
//     // Needed for microseconds which we convert to milliseconds
//     struct timeval tv;
//     gettimeofday(&tv, NULL);

//     return tv.tv_sec * 1000000 + tv.tv_usec;
// #else
//     return 0L;
// #endif
// }

/**
 *
 * This is the central cpp method called by the Java TOOL to run vision
 * results. It takes in the raw image data, as well as the joint angles
 * and single byte array representation of the color table.
 * (possibly later it will also need to take other sensor values.)
 *
 * Due to the difficulty of JNI, we currently also require that the thresholded
 * array which Java wants filled be passed in as well. This removes the need
 * for us to construct a java byte[][] from cpp.
 *
 * This method explicitly returns nothing. The results of vision computation
 * are sent back to Java via some setter methods. Right now we only send back
 * the thresholded array after vision is done with it. In the future,
 * we should make a method to translate a cpp field object into a
 * Data/FieldObject.java compatible field object.
 *
 * KNOWN INEFFICIENCIES:In the future, we'd like to be able
 * to store a static file-level pointer to the vision class. A method which
 * instantiates vision  be called in the TOOLVisionLink.java  contructor.
 * This instance of vision will also need to be destroyed somehow when
 * we are done with this VisionLink
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

    //Instantiate the vision stuff
    static shared_ptr<Sensors> sensors(new Sensors());
    static shared_ptr<NaoPose> pose(new NaoPose(sensors));
    static shared_ptr<Profiler> profiler(new Profiler(micro_time));
    static Vision vision(pose, profiler);

    JNIEXPORT void JNICALL Java_TOOL_Vision_TOOLVisionLink_cppProcessImage
    (JNIEnv * env, jobject jobj, jbyteArray jimg, jfloatArray jjoints,
     jfloatArray jsensors, jbyteArray jtable, jobjectArray thresh_target){
        //Size checking -- we expect the sizes of the arrays to match
        //Base these on the size cpp expects for the image
        unsigned int tlenw =
            env->GetArrayLength((jbyteArray)
                                env->GetObjectArrayElement(thresh_target,0));
        //If one of the dimensions is wrong, we exit
        if(env->GetArrayLength(jimg) != IMAGE_BYTE_SIZE) {
            cout << "Error: the image had the wrong byte size" << endl;
            cout << "Image byte size should be " << IMAGE_BYTE_SIZE << endl;
            cout << "Detected byte size of " << env->GetArrayLength(jimg)
                 << endl;
            return;
        }
        if (env->GetArrayLength(jjoints) != Kinematics::NUM_JOINTS) {
            cout << "Error: the joint array had incorrect dimensions" << endl;
            return;
        }
        if (env->GetArrayLength(jsensors) != NUM_SENSORS) {
            cout << "Error: the sensors array had incorrect dimensions" << endl;
            return;
        }
        if (env->GetArrayLength(jtable) != YMAX*UMAX*VMAX) {
            cout << "Error: the color table had incorrect dimensions" << endl;
            return;
        }
        if (env->GetArrayLength(thresh_target) != IMAGE_HEIGHT ||
            tlenw != IMAGE_WIDTH) {
            cout << "Error: the thresh_target had incorrect dimensions" << endl;
            return;
        }

        //load the table
        jbyte *buf_table = env->GetByteArrayElements( jtable, 0);
        byte * table = (byte *)buf_table; //convert it to a reg. byte array
        vision.thresh->initTableFromBuffer(table);
        env->ReleaseByteArrayElements( jtable, buf_table, 0);

        // Set the joints data - Note: set visionBodyAngles not bodyAngles
        float * joints = env->GetFloatArrayElements(jjoints,0);
        vector<float> joints_vector(&joints[0],&joints[22]);
        env->ReleaseFloatArrayElements(jjoints,joints,0);
        sensors->setVisionBodyAngles(joints_vector);

        // Set the sensor data
        float * sensors_array = env->GetFloatArrayElements(jsensors,0);
        vector<float> sensors_vector(&sensors_array[0],&sensors_array[22]);
        env->ReleaseFloatArrayElements(jsensors,sensors_array,0);
        sensors->setAllSensors(sensors_vector);

        // Clear the debug image on which the vision algorithm can draw
        vision.thresh->initDebugImage();

        //get pointer access to the java image array
        jbyte *buf_img = env->GetByteArrayElements( jimg, 0);
        byte * img = (byte *)buf_img; //convert it to a reg. byte array
        //PROCESS VISION!!
        vision.notifyImage(img);
        vision.drawBoxes();
        env->ReleaseByteArrayElements( jimg, buf_img, 0);

        //Debug output:
        /*
        cout <<"Ball Width: "<<  vision.ball->getWidth() <<endl;
        cout<<"Pose Left Hor Y" << pose->getLeftHorizonY() <<endl;
        cout<<"Pose Right Hor Y" << pose->getRightHorizonY() <<endl;
        */
        //copy results from vision thresholded to the array passed in from java
        //we access to each row in the java array, and copy in from cpp thresholded
        //we may in the future want to experiment with malloc, for increased speed
        for(int i = 0; i < IMAGE_HEIGHT; i++){
            jbyteArray row_target=
                (jbyteArray) env->GetObjectArrayElement(thresh_target,i);
            jbyte* row = env->GetByteArrayElements(row_target,0);

            for(int j = 0; j < IMAGE_WIDTH; j++){
                row[j]= vision.thresh->thresholded[i][j];
            }
            env->ReleaseByteArrayElements(row_target, row, 0);
        }

        return;

    }

    JNIEXPORT void JNICALL Java_TOOL_Vision_TOOLVisionLink_cppPixEstimate
        (JNIEnv * env, jobject jobj, jint pixelX, jint pixelY,
         jfloat objectHeight, jdoubleArray estimateResult) {
            // make sure the array for the estimate is big enough. There
            // should be room for five things in there. (subject to change)
            if (env->GetArrayLength(estimateResult) !=
                sizeof(estimate)/sizeof(double)) {
                cout << "Error: the estimateResult array had incorrect "
                    "dimensions" << endl;
                return;
            }

            //load the table
            jdouble * buf_estimate =
                env->GetDoubleArrayElements( estimateResult, 0);
            double * estimate_array =
                (double *)buf_estimate;
            //vision.thresh->initTableFromBuffer(table);
            estimate est = pose->pixEstimate(static_cast<int>(pixelX),
                                             static_cast<int>(pixelY),
                                             static_cast<float>(objectHeight));

            estimate_array[0] = est.dist;
            estimate_array[1] = est.elevation;
            estimate_array[2] = est.bearing;
            estimate_array[3] = est.x;
            estimate_array[4] = est.y;

            env->ReleaseDoubleArrayElements( estimateResult, buf_estimate, 0);
    }

#ifdef __cplusplus
}
#endif

