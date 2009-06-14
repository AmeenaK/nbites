#ifndef InverseKinematics_h
#define InverseKinematics_h

#include "Kinematics.h"
namespace Kinematics{
    /*
     * Declarations for constants and methods concerning forward and inverse
     * kinematics.
     */
    //Accuracy constants for dls
    //in mm, how close dls will get to the target
    static const float UNBELIEVABLY_LOW_ERROR = 0.01f; //mm
    static const float REALLY_LOW_ERROR = 0.1f; //mm
    static const float ACCEPTABLE_ERROR = 0.5f; //mm
    static const float COARSE_ERROR     = 1.0f; //mm

    static const float dampFactor = 0.4f;
    static const float maxDeltaTheta = 0.5f;
    static const int maxAnkleIterations = 60;
    static const int maxHeelIterations = 20;


    enum IKOutcome {
        STUCK = 0,
        SUCCESS = 1
    };

    struct IKLegResult {
        IKOutcome outcome;
        float angles[6];
    };

    const void clipChainAngles(const ChainID id,
                               float angles[]);
    const float getMinValue(const ChainID id, const int jointNumber);
    const float getMaxValue(const ChainID id, const int jointNumber);
    const NBMath::ufvector3 forwardKinematics(const ChainID id,
                                              const float angles[]);
    const NBMath::ufmatrix3 buildJacobians(const ChainID chainID,
                                              const float angles[]);


    // Both adjustment methods return whether the search was successful.
    // The correct angles required to fulfill the goal are returned through
    // startAngles by reference.
    const bool adjustAnkle(const ChainID chainID,
                           const NBMath::ufvector3 &goal,
                           float startAngles[],
                           const float maxError);
    const bool adjustHeel(const ChainID chainID,
                          const NBMath::ufvector3 &goal,
                          float startAngles[],
                          const float maxError);
    const IKLegResult dls(const ChainID chainID,
                          const NBMath::ufvector3 &goal,
                          const float startAngles[],
                          const float maxError = ACCEPTABLE_ERROR,
                          const float maxHeelError = UNBELIEVABLY_LOW_ERROR);


//Depricated methods

    const NBMath::ufmatrix3 buildLegJacobian(const ChainID chainID,
                                              const float angles[]);

    const NBMath::ufmatrix3 buildHeelJacobian(const ChainID chainID,
                                              const float angles[]);
    void hackJointOrder(float angles[]);

};
#endif
