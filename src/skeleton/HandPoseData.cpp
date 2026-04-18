#include "HandPoseData.h"

using frik::FingerPose;
using frik::HandFingersPose;
using frik::skeleton::data::HandBonePoseData;
using frik::skeleton::data::RotationData;

namespace
{
    constexpr HandFingersPose GUN_GRIP_POSE{
        FingerPose{ 0.7f, 0.4f, 0.5f },
        FingerPose{ 0.9f, 0.6f, 0.5f },
        FingerPose{ 0.3f, 0.5f, 0.5f },
        FingerPose{ 0.1f, 0.5f, 0.5f },
        FingerPose{ 0.0f, 0.5f, 0.7f }
    };

    constexpr HandFingersPose MELEE_GRIP_POSE{
        FingerPose{ 0.7f, 0.5f, 0.8f },
        FingerPose{ 0.4f, 0.3f, 0.9f },
        FingerPose{ 0.1f, 0.5f, 0.9f },
        FingerPose{ 0.0f, 0.5f, 0.9f },
        FingerPose{ 0.0f, 0.4f, 0.9f }
    };

    constexpr HandFingersPose POINTING_POSE{
        FingerPose{ 0.0f, 0.0f, 0.0f },
        FingerPose{ 1.0f, 1.0f, 1.0f },
        FingerPose{ 0.0f, 0.0f, 0.0f },
        FingerPose{ 0.0f, 0.0f, 0.0f },
        FingerPose{ 0.0f, 0.0f, 0.0f }
    };

    constexpr HandFingersPose ATTABOY_POSE{
        FingerPose{ 0.7f, 1.2f, 1.3f },
        FingerPose{ 1.1f, 1.1f, 1.2f },
        FingerPose{ 0.8f, 0.6f, 1.0f },
        FingerPose{ 0.4f, 0.8f, 1.0f },
        FingerPose{ 0.1f, 1.0f, 1.4f }
    };

    constexpr HandFingersPose OFFHAND_WEAPON_GRIP_POSE{
        FingerPose{ 1.0f, 1.0f, 0.9f },
        FingerPose{ 0.6f, 0.6f, 0.6f },
        FingerPose{ 0.5f, 0.6f, 0.55f },
        FingerPose{ 0.5f, 0.5f, 0.5f },
        FingerPose{ 0.5f, 0.5f, 0.5f }
    };

    constexpr HandFingersPose THUMBS_UP_POSE{
        FingerPose{ 1.6f, 1.4f, 1.5f, 0.5f },
        FingerPose{ 0.0f },
        FingerPose{ 0.0f },
        FingerPose{ 0.0f },
        FingerPose{ 0.0f }
    };

    constexpr HandFingersPose FIST_POSE{
        FingerPose{ 0.0f },
        FingerPose{ 0.0f },
        FingerPose{ 0.0f },
        FingerPose{ 0.0f },
        FingerPose{ 0.0f }
    };

    const std::array<HandBonePoseData, 30> HAND_BONE_DATA = { {
        HandBonePoseData{ .boneName = "LArm_Finger11",
                          .closedRotation = RotationData{ 0.8494F, -0.2706F, 0.4531F, 0.0000F, -0.3826F, 0.2755F, 0.8819F, 0.0000F, -0.3635F, -0.9224F, 0.1305F, 0.0000F },
                          .openRotation = RotationData{ 0.6177F, -0.4004F, 0.6768F, 0.0000F, -0.6540F, 0.2164F, 0.7249F, 0.0000F, -0.4367F, -0.8904F, -0.1282F, 0.0000F },
                          .openTranslation = RE::NiPoint3(1.583F, -1.263F, 1.853F), .openTranslationInPowerArmor = RE::NiPoint3(3.993F, -4.156F, 3.586F) },
        HandBonePoseData{ .boneName = "LArm_Finger12",
                          .closedRotation = RotationData{ 0.6985F, -0.7139F, 0.0489F, 0.0000F, 0.7105F, 0.7001F, 0.0707F, 0.0000F, -0.0847F, -0.0146F, 0.9963F, 0.0000F },
                          .openRotation = RotationData{ 0.8995F, -0.4343F, -0.0484F, 0.0000F, 0.4355F, 0.9000F, 0.0194F, 0.0000F, 0.0351F, -0.0385F, 0.9986F, 0.0000F },
                          .openTranslation = RE::NiPoint3(3.570F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(2.894F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "LArm_Finger13",
                          .closedRotation = RotationData{ 0.1252F, -0.9921F, -0.0064F, 0.0000F, 0.9910F, 0.1253F, -0.0480F, 0.0000F, 0.0485F, -0.0004F, 0.9988F, 0.0000F },
                          .openRotation = RotationData{ 0.9457F, -0.3218F, -0.0458F, 0.0000F, 0.3214F, 0.9468F, -0.0153F, 0.0000F, 0.0483F, -0.0003F, 0.9988F, 0.0000F },
                          .openTranslation = RE::NiPoint3(2.402F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(4.687F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "LArm_Finger21",
                          .closedRotation = RotationData{ 0.0890F, -0.9952F, 0.0408F, 0.0000F, 0.9952F, 0.0906F, 0.0382F, 0.0000F, -0.0418F, 0.0372F, 0.9984F, 0.0000F },
                          .openRotation = RotationData{ 0.9903F, -0.1148F, 0.0788F, 0.0000F, 0.1112F, 0.9926F, 0.0480F, 0.0000F, -0.0838F, -0.0388F, 0.9957F, 0.0000F },
                          .openTranslation = RE::NiPoint3(7.501F, 0.430F, 2.278F), .openTranslationInPowerArmor = RE::NiPoint3(8.475F, -2.161F, 3.790F) },
        HandBonePoseData{ .boneName = "LArm_Finger22",
                          .closedRotation = RotationData{ -0.4736F, -0.8807F, 0.0000F, 0.0000F, 0.8807F, -0.4736F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9583F, -0.2858F, 0.0000F, 0.0000F, 0.2858F, 0.9583F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(3.018F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(2.613F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "LArm_Finger23",
                          .closedRotation = RotationData{ -0.1231F, -0.9924F, 0.0000F, 0.0000F, 0.9924F, -0.1231F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9924F, -0.1234F, 0.0000F, 0.0000F, 0.1234F, 0.9924F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(1.850F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(5.146F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "LArm_Finger31",
                          .closedRotation = RotationData{ 0.1593F, -0.9829F, 0.0927F, 0.0000F, 0.9839F, 0.1504F, -0.0967F, 0.0000F, 0.0811F, 0.1066F, 0.9910F, 0.0000F },
                          .openRotation = RotationData{ 0.9517F, -0.2761F, -0.1346F, 0.0000F, 0.2670F, 0.9602F, -0.0820F, 0.0000F, 0.1519F, 0.0421F, 0.9875F, 0.0000F },
                          .openTranslation = RE::NiPoint3(7.596F, 0.621F, 0.457F), .openTranslationInPowerArmor = RE::NiPoint3(8.152F, -2.577F, 1.100F) },
        HandBonePoseData{ .boneName = "LArm_Finger32",
                          .closedRotation = RotationData{ -0.4566F, -0.8897F, 0.0000F, 0.0000F, 0.8897F, -0.4566F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9025F, -0.4306F, -0.0002F, 0.0000F, 0.4306F, 0.9025F, 0.0007F, 0.0000F, -0.0002F, -0.0007F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(3.092F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(3.723F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "LArm_Finger33",
                          .closedRotation = RotationData{ -0.0767F, -0.9971F, 0.0000F, 0.0000F, 0.9971F, -0.0767F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9531F, -0.3025F, 0.0001F, 0.0000F, 0.3025F, 0.9531F, -0.0007F, 0.0000F, 0.0001F, 0.0007F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(2.188F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(4.984F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "LArm_Finger41",
                          .closedRotation = RotationData{ 0.1230F, -0.9783F, 0.1665F, 0.0000F, 0.9783F, 0.0914F, -0.1858F, 0.0000F, 0.1665F, 0.1858F, 0.9684F, 0.0000F },
                          .openRotation = RotationData{ 0.9190F, -0.3923F, -0.0385F, 0.0000F, 0.3844F, 0.9136F, -0.1323F, 0.0000F, 0.0871F, 0.1068F, 0.9905F, 0.0000F },
                          .openTranslation = RE::NiPoint3(7.464F, 0.350F, -1.439F), .openTranslationInPowerArmor = RE::NiPoint3(7.968F, -2.259F, -1.337F) },
        HandBonePoseData{ .boneName = "LArm_Finger42",
                          .closedRotation = RotationData{ -0.3667F, -0.9303F, 0.0000F, 0.0000F, 0.9303F, -0.3667F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9270F, -0.3750F, 0.0000F, 0.0000F, 0.3750F, 0.9270F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(2.664F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(2.934F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "LArm_Finger43",
                          .closedRotation = RotationData{ 0.3242F, -0.9460F, 0.0000F, 0.0000F, 0.9460F, 0.3242F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9850F, -0.1727F, 0.0000F, 0.0000F, 0.1727F, 0.9850F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(1.900F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(5.103F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "LArm_Finger51",
                          .closedRotation = RotationData{ 0.2045F, -0.9360F, 0.2866F, 0.0000F, 0.9528F, 0.1232F, -0.2776F, 0.0000F, 0.2245F, 0.3299F, 0.9169F, 0.0000F },
                          .openRotation = RotationData{ 0.8260F, -0.5570F, -0.0867F, 0.0000F, 0.5349F, 0.8230F, -0.1911F, 0.0000F, 0.1778F, 0.1115F, 0.9777F, 0.0000F },
                          .openTranslation = RE::NiPoint3(6.637F, -0.357F, -3.018F), .openTranslationInPowerArmor = RE::NiPoint3(8.365F, -2.603F, -3.706F) },
        HandBonePoseData{ .boneName = "LArm_Finger52",
                          .closedRotation = RotationData{ -0.1904F, -0.9817F, -0.0004F, 0.0000F, 0.9817F, -0.1904F, -0.0005F, 0.0000F, 0.0004F, -0.0005F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9360F, -0.3521F, 0.0000F, 0.0000F, 0.3521F, 0.9360F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(2.238F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(2.128F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "LArm_Finger53",
                          .closedRotation = RotationData{ -0.1882F, -0.9821F, 0.0000F, 0.0000F, 0.9821F, -0.1882F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.8336F, -0.5523F, 0.0000F, 0.0000F, 0.5523F, 0.8336F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(1.666F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(4.594F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger11",
                          .closedRotation = RotationData{ 0.7521F, -0.2827F, -0.5954F, 0.0000F, -0.3977F, 0.5257F, -0.7520F, 0.0000F, 0.5256F, 0.8023F, 0.2829F, 0.0000F },
                          .openRotation = RotationData{ 0.5849F, -0.4006F, -0.7053F, 0.0000F, -0.6564F, 0.2770F, -0.7017F, 0.0000F, 0.4765F, 0.8734F, -0.1009F, 0.0000F },
                          .openTranslation = RE::NiPoint3(1.583F, -1.263F, -1.853F), .openTranslationInPowerArmor = RE::NiPoint3(3.993F, -4.156F, -3.586F) },
        HandBonePoseData{ .boneName = "RArm_Finger12",
                          .closedRotation = RotationData{ 0.5562F, -0.8303F, -0.0356F, 0.0000F, 0.8267F, 0.5571F, -0.0784F, 0.0000F, 0.0850F, 0.0142F, 0.9963F, 0.0000F },
                          .openRotation = RotationData{ 0.8122F, -0.5833F, 0.0000F, 0.0000F, 0.5833F, 0.8122F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(3.570F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(2.894F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger13",
                          .closedRotation = RotationData{ 0.6207F, -0.7834F, 0.0302F, 0.0000F, 0.7825F, 0.6215F, 0.0376F, 0.0000F, -0.0482F, 0.0003F, 0.9988F, 0.0000F },
                          .openRotation = RotationData{ 0.9704F, -0.2414F, 0.0000F, 0.0000F, 0.2414F, 0.9704F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(2.402F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(4.687F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger21",
                          .closedRotation = RotationData{ 0.3870F, -0.9154F, -0.1113F, 0.0000F, 0.9177F, 0.3941F, -0.0504F, 0.0000F, 0.0900F, -0.0827F, 0.9925F, 0.0000F },
                          .openRotation = RotationData{ 0.9693F, -0.2046F, -0.1361F, 0.0000F, 0.1955F, 0.9776F, -0.0775F, 0.0000F, 0.1489F, 0.0485F, 0.9877F, 0.0000F },
                          .openTranslation = RE::NiPoint3(7.501F, 0.430F, -2.278F), .openTranslationInPowerArmor = RE::NiPoint3(8.474F, -2.161F, -3.790F) },
        HandBonePoseData{ .boneName = "RArm_Finger22",
                          .closedRotation = RotationData{ -0.1520F, -0.9884F, 0.0000F, 0.0000F, 0.9884F, -0.1520F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9495F, -0.3138F, 0.0000F, 0.0000F, 0.3138F, 0.9495F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(3.018F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(2.613F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger23",
                          .closedRotation = RotationData{ 0.3976F, -0.9176F, 0.0000F, 0.0000F, 0.9176F, 0.3976F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9802F, -0.1980F, 0.0000F, 0.0000F, 0.1980F, 0.9802F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(1.850F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(5.145F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger31",
                          .closedRotation = RotationData{ 0.0767F, -0.9920F, -0.1002F, 0.0000F, 0.9968F, 0.0785F, -0.0147F, 0.0000F, 0.0224F, -0.0987F, 0.9949F, 0.0000F },
                          .openRotation = RotationData{ 0.9542F, -0.2989F, 0.0125F, 0.0000F, 0.2978F, 0.9530F, 0.0557F, 0.0000F, -0.0285F, -0.0494F, 0.9984F, 0.0000F },
                          .openTranslation = RE::NiPoint3(7.596F, 0.621F, -0.457F), .openTranslationInPowerArmor = RE::NiPoint3(8.152F, -2.577F, -1.100F) },
        HandBonePoseData{ .boneName = "RArm_Finger32",
                          .closedRotation = RotationData{ -0.0684F, -0.9977F, 0.0000F, 0.0000F, 0.9977F, -0.0684F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9034F, -0.4287F, 0.0000F, 0.0000F, 0.4287F, 0.9034F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(3.092F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(3.723F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger33",
                          .closedRotation = RotationData{ -0.0501F, -0.9987F, 0.0000F, 0.0000F, 0.9987F, -0.0501F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9677F, -0.2521F, 0.0000F, 0.0000F, 0.2521F, 0.9677F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(2.188F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(4.974F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger41",
                          .closedRotation = RotationData{ 0.0682F, -0.9827F, -0.1722F, 0.0000F, 0.9977F, 0.0681F, 0.0069F, 0.0000F, 0.0049F, -0.1722F, 0.9850F, 0.0000F },
                          .openRotation = RotationData{ 0.9263F, -0.3767F, -0.0028F, 0.0000F, 0.3722F, 0.9140F, 0.1615F, 0.0000F, -0.0583F, -0.1507F, 0.9869F, 0.0000F },
                          .openTranslation = RE::NiPoint3(7.464F, 0.350F, 1.439F), .openTranslationInPowerArmor = RE::NiPoint3(7.968F, -2.259F, 1.337F) },
        HandBonePoseData{ .boneName = "RArm_Finger42",
                          .closedRotation = RotationData{ 0.0935F, -0.9956F, 0.0000F, 0.0000F, 0.9956F, 0.0935F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9143F, -0.4049F, 0.0000F, 0.0000F, 0.4049F, 0.9143F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(2.664F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(2.934F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger43",
                          .closedRotation = RotationData{ -0.3352F, -0.9421F, 0.0000F, 0.0000F, 0.9421F, -0.3352F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9191F, -0.3939F, 0.0000F, 0.0000F, 0.3939F, 0.9191F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(1.900F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(5.102F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger51",
                          .closedRotation = RotationData{ 0.2571F, -0.9316F, -0.2571F, 0.0000F, 0.9560F, 0.2063F, 0.2086F, 0.0000F, -0.1413F, -0.2994F, 0.9436F, 0.0000F },
                          .openRotation = RotationData{ 0.9216F, -0.3766F, 0.0934F, 0.0000F, 0.3460F, 0.9066F, 0.2416F, 0.0000F, -0.1757F, -0.1903F, 0.9659F, 0.0000F },
                          .openTranslation = RE::NiPoint3(6.637F, -0.357F, 3.018F), .openTranslationInPowerArmor = RE::NiPoint3(8.365F, -2.603F, 3.707F) },
        HandBonePoseData{ .boneName = "RArm_Finger52",
                          .closedRotation = RotationData{ -0.2120F, -0.9773F, -0.0004F, 0.0000F, 0.9773F, -0.2120F, -0.0005F, 0.0000F, 0.0004F, -0.0005F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.9571F, -0.2898F, 0.0000F, 0.0000F, 0.2898F, 0.9571F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(2.238F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(2.128F, 0.000F, 0.000F) },
        HandBonePoseData{ .boneName = "RArm_Finger53",
                          .closedRotation = RotationData{ -0.2765F, -0.9610F, 0.0000F, 0.0000F, 0.9610F, -0.2765F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openRotation = RotationData{ 0.7585F, -0.6517F, 0.0000F, 0.0000F, 0.6517F, 0.7585F, 0.0000F, 0.0000F, 0.0000F, 0.0000F, 1.0000F, 0.0000F },
                          .openTranslation = RE::NiPoint3(1.666F, 0.000F, 0.000F), .openTranslationInPowerArmor = RE::NiPoint3(4.594F, 0.000F, 0.000F) },
    } };
}

namespace frik::skeleton::data
{
    const HandFingersPose& getGunGripPose() noexcept
    {
        return GUN_GRIP_POSE;
    }

    const HandFingersPose& getMeleeGripPose() noexcept
    {
        return MELEE_GRIP_POSE;
    }

    const HandFingersPose& getPointingPose() noexcept
    {
        return POINTING_POSE;
    }

    const HandFingersPose& getAttaboyPose() noexcept
    {
        return ATTABOY_POSE;
    }

    const HandFingersPose& getOffhandWeaponGripPose() noexcept
    {
        return OFFHAND_WEAPON_GRIP_POSE;
    }

    const HandFingersPose& getThumbsUpPose() noexcept
    {
        return THUMBS_UP_POSE;
    }

    const HandFingersPose& getFistPose() noexcept
    {
        return FIST_POSE;
    }

    const std::array<HandBonePoseData, 30>& getHandBoneData() noexcept
    {
        return HAND_BONE_DATA;
    }
}
