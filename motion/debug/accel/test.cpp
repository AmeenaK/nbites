#include<iostream>
#include <cstdio>
using namespace std;
#include "AccEKF.h"
#include "ZmpEKF.h"
#include "ZmpAccEKF.h"
#include "BasicWorldConstants.h"

/**
 * This is a test file to parse logs and test the various kalman filters we
 * use for motion, etc
 */

void AccFilter(FILE *f){

    AccEKF a;

    FILE *output = fopen("/tmp/accel_log.xls","w");
    fprintf(output,
            "time\taccX\taccY\taccZ\tfilteredAccX\tfilteredAccY\tfilteredAccZ\tfilteredAccXUnc\tfilteredAccYUnc\tfilteredAccZUnc\n");
    while (!feof(f)) {
        float accX,accY,accZ;
        fscanf(f,"%f\t%f\t%f\n",&accX,&accY,&accZ);
        a.update(accX,accY,accZ);
        float fAccX = a.getX();  float fAccY = a.getY();float fAccZ = a.getZ();
        float fAccXUnc = a.getXUnc();
        float fAccYUnc = a.getYUnc();
        float fAccZUnc = a.getZUnc();
        static float time = 0.0;
        fprintf(output,"%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
                time,accX,accY,accZ,fAccX,fAccY,fAccZ,
                fAccXUnc, fAccYUnc, fAccZUnc);
        time +=.02;
    }

}


void ZmpFilter(FILE *f){

    // parse the header of the file
    cout << fscanf(f,"time\tcom_x\tcom_y\tpre_x\tpre_y\tzmp_x\tzmp_y\t"
            "sensor_zmp_x\tsensor_zmp_y\treal_com_x\treal_com_y\tangleX\t"
            "angleY\taccX\taccY\taccZ\t"
            "lfl\tlfr\tlrl\tlrr\trfl\trfr\trrl\trrr\t"
            "state\n") <<endl;;

    ZmpEKF a;

    FILE *output = fopen("/tmp/zmp_log.xls","w");
    fprintf(output,"time\tcom_x\tcom_y\tpre_x\tpre_y\tzmp_x\tzmp_y\t"
            "filtered_zmp_x\tfiltered_zmp_y\tunfiltered_zmp_x\tunfiltered_zmp_y\n");
    while (!feof(f)) {

        float comX,comY,zmpX,zmpY,preX,preY,estZMPX,estZMPY;
        float angleX,angleY,accX =0.0f,accY=0.0f,accZ;
        float time;
        float bogus;
        int bogi;
        fscanf(f,"%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t"
               "%f\t%f\t%f\t"
               "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%d\n",
               &time,&comX,&comY,&preX,&preY,&zmpX,&zmpY,
               &estZMPX,&estZMPY,&bogus,&bogus, &angleX,&angleY,
               &accX,&accY,&accZ,
               &bogus,&bogus,&bogus,&bogus,&bogus,&bogus,&bogus,&bogus,&bogi);

        //Note the unit conversion from m to mm we are doing implicitly
        estZMPX = comX + 310/GRAVITY_mss * accX;
        estZMPY = comY + 310/GRAVITY_mss * accY;

        ZmpTimeUpdate tUp = {zmpX,zmpY};
        ZmpMeasurement pMeasure = {comX,comY,accX,accY};
        a.update(tUp,pMeasure);

        float filtered_zmp_x = a.get_zmp_x();
        float filtered_zmp_y = a.get_zmp_y();
        float filtered_zmp_unc_x = a.get_zmp_unc_x();
        float filtered_zmp_unc_y = a.get_zmp_unc_y();

        fprintf(output, "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
                time,
                comX, comY, preX, preY, zmpX, zmpY,
                filtered_zmp_x, filtered_zmp_y,
                estZMPX, estZMPY);
                //filtered_zmp_unc_x, filtered_zmp_unc_y,

    }

}

void ZmpAccFilter(FILE * f){

    //Read the header from the input log file
    fscanf(f,"time\tpre_x\tpre_y\tcom_x\tcom_y\tcom_px\tcom_py"
           "\taccX\taccY\taccZ\n");

    //Write the header for the accout log file
    FILE * accout = fopen("/tmp/zmpacc_log.xls","w");
    fprintf(accout,"time\taccX\taccY\taccZ\t"
            "filteredAccX\tfilteredAccY\tfilteredAccZ\t"
            "filteredAccXUnc\tfilteredAccYUnc\tfilteredAccZUnc\n");

    FILE * zmpout = fopen("/tmp/zmpekf_log.xls","w");
    fprintf(zmpout,"time\tcom_x\tcom_y\tcom_px\tcom_py\tpre_x\tpre_y\t"
            "sensor_px\tsensor_py\tsensor_px_unf\tsensor_py_unf\t"
            "sensor_px_unff\tsensor_py_unff\n");

    ZmpEKF zmpFilter;
    ZmpAccEKF accFilter;

    while (!feof(f)) {
        //Read the input log file
        float time,preX,preY,comX,comY,comPX,comPY,accX,accY,accZ;
        int nScan = fscanf(f,"%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
                           &time,&preX,&preY,&comX,&comY,&comPX,&comPY,
                           &accX,&accY,&accZ);

        //Process the accel values using EKF:
        accFilter.update(accX,accY,accZ);
        const float fAccX = accFilter.getX();
        const float fAccY = accFilter.getY();
        const float fAccZ = accFilter.getZ();
        float fAccXUnc = accFilter.getXUnc();
        float fAccYUnc = accFilter.getYUnc();
        float fAccZUnc = accFilter.getZUnc();

        //Process the accel values into ZMP estimates using an EKF:
        ZmpTimeUpdate tUp = {comPX,comPY};
        ZmpMeasurement pMeasure = {comX,comY,fAccX,fAccY};
        zmpFilter.update(tUp,pMeasure);
        const float sensorPX = zmpFilter.get_zmp_x();
        const float sensorPY = zmpFilter.get_zmp_y();

        //slightly filtered ZMP est
        const float sensorPXUnf = comX + (310.0f / GRAVITY_mss )*fAccX;
        const float sensorPYUnf = comY + (310.0f / GRAVITY_mss )*fAccY;

        //Completely unfiltered ZMP est
        const float sensorPXUnff = comX + (310.0f / GRAVITY_mss )*accX;
        const float sensorPYUnff = comY + (310.0f / GRAVITY_mss )*accY;

        //Write accels to a new log file
        fprintf(accout,
                "%f\t"
                "%f\t%f\t%f\t"
                "%f\t%f\t%f\t"
                "%f\t%f\t%f\n",
                time,
                accX,accY,accZ,
                fAccX,fAccY,fAccZ,
                fAccXUnc,fAccYUnc,fAccZUnc);

        fprintf(zmpout, "%f\t"
                "%f\t%f\t%f\t%f\t"
                "%f\t%f\t%f\t%f\t"
                "%f\t%f\t%f\t%f\n",
                time,
                comX,comY,comPX,comPY,
                preX,preY,sensorPX,sensorPY,
                sensorPXUnf,sensorPYUnf,
                sensorPXUnff,sensorPYUnff);
    }


    //Close the log file
    fclose(accout);
}


int main(int argc, char * argv[]) {
    if (argc < 2){
        cout << "Not enough arguments" <<endl;
        return 1;
    }
    FILE *f = fopen(argv[1],"r");
    //ZmpFilter(f);
    ZmpAccFilter(f);

    return 0;
}
