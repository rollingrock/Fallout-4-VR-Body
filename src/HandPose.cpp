#include "HandPose.h"
#include <map>
#include <numbers>
#include <string>
#include "Config.h"
#include "f4se/NiTypes.h"

using namespace common;

// TODO: this code is terrible, primary it doesn't handle multiple code paths set hand pose, release will release all of them
namespace frik
{
    std::map<std::string, RE::NiTransform, CaseInsensitiveComparator> handClosed;
    std::map<std::string, RE::NiTransform, CaseInsensitiveComparator> handOpen;

    std::map<std::string, float> handPapyrusPose;
    std::map<std::string, bool> handPapyrusHasControl;

    constexpr int FINGERS_COUNT = 15;

    static const std::string LEFT_HAND_FINGERS[] = {
        "LArm_Finger11", "LArm_Finger12", "LArm_Finger13", "LArm_Finger21", "LArm_Finger22", "LArm_Finger23", "LArm_Finger31", "LArm_Finger32", "LArm_Finger33", "LArm_Finger41",
        "LArm_Finger42", "LArm_Finger43", "LArm_Finger51", "LArm_Finger52", "LArm_Finger53"
    };
    static const std::string RIGHT_HAND_FINGERS[] = {
        "RArm_Finger11", "RArm_Finger12", "RArm_Finger13", "RArm_Finger21", "RArm_Finger22", "RArm_Finger23", "RArm_Finger31", "RArm_Finger32", "RArm_Finger33", "RArm_Finger41",
        "RArm_Finger42", "RArm_Finger43", "RArm_Finger51", "RArm_Finger52", "RArm_Finger53"
    };
    static constexpr float HAND_FINGERS_POINTING_POSE[] = { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    static bool _handPointingPoseSet[2] = { false, false };

    static bool _offHandGripPose = false;
    static constexpr float OFFHAND_FINGERS_GRIP_POSE[] = { 1.0f, 1.0f, 0.9f, 0.6f, 0.6f, 0.6f, 0.5f, 0.6f, 0.55f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };

    static void copyDataIntoHand(std::vector<std::vector<float>> data, std::map<std::string, RE::NiTransform, CaseInsensitiveComparator>& hand)
    {
        std::ranges::copy(data[0], hand["LArm_Finger11"].rotate.arr);
        std::ranges::copy(data[1], hand["LArm_Finger12"].rotate.arr);
        std::ranges::copy(data[2], hand["LArm_Finger13"].rotate.arr);
        std::ranges::copy(data[3], hand["LArm_Finger21"].rotate.arr);
        std::ranges::copy(data[4], hand["LArm_Finger22"].rotate.arr);
        std::ranges::copy(data[5], hand["LArm_Finger23"].rotate.arr);
        std::ranges::copy(data[6], hand["LArm_Finger31"].rotate.arr);
        std::ranges::copy(data[7], hand["LArm_Finger32"].rotate.arr);
        std::ranges::copy(data[8], hand["LArm_Finger33"].rotate.arr);
        std::ranges::copy(data[9], hand["LArm_Finger41"].rotate.arr);
        std::ranges::copy(data[10], hand["LArm_Finger42"].rotate.arr);
        std::ranges::copy(data[11], hand["LArm_Finger43"].rotate.arr);
        std::ranges::copy(data[12], hand["LArm_Finger51"].rotate.arr);
        std::ranges::copy(data[13], hand["LArm_Finger52"].rotate.arr);
        std::ranges::copy(data[14], hand["LArm_Finger53"].rotate.arr);
        std::ranges::copy(data[15], hand["RArm_Finger11"].rotate.arr);
        std::ranges::copy(data[16], hand["RArm_Finger12"].rotate.arr);
        std::ranges::copy(data[17], hand["RArm_Finger13"].rotate.arr);
        std::ranges::copy(data[18], hand["RArm_Finger21"].rotate.arr);
        std::ranges::copy(data[19], hand["RArm_Finger22"].rotate.arr);
        std::ranges::copy(data[20], hand["RArm_Finger23"].rotate.arr);
        std::ranges::copy(data[21], hand["RArm_Finger31"].rotate.arr);
        std::ranges::copy(data[22], hand["RArm_Finger32"].rotate.arr);
        std::ranges::copy(data[23], hand["RArm_Finger33"].rotate.arr);
        std::ranges::copy(data[24], hand["RArm_Finger41"].rotate.arr);
        std::ranges::copy(data[25], hand["RArm_Finger42"].rotate.arr);
        std::ranges::copy(data[26], hand["RArm_Finger43"].rotate.arr);
        std::ranges::copy(data[27], hand["RArm_Finger51"].rotate.arr);
        std::ranges::copy(data[28], hand["RArm_Finger52"].rotate.arr);
        std::ranges::copy(data[29], hand["RArm_Finger53"].rotate.arr);
    }

    void initHandPoses(const bool inPowerArmor)
    {
        std::vector<std::vector<float>> data;

        // pulled from the game engine while running idle animations

        //closed fist first
        data.push_back({ 0.849409F, -0.270577F, 0.453092F, 0, -0.382631F, 0.275533F, 0.881859F, 0, -0.363453F, -0.922426F, 0.130509F, 0 });
        data.push_back({ 0.698533F, -0.713903F, 0.048938F, 0, 0.710545F, 0.700093F, 0.070685F, 0, -0.084723F, -0.014603F, 0.996297F, 0 });
        data.push_back({ 0.125157F, -0.992116F, -0.006447F, 0, 0.990953F, 0.125323F, -0.048036F, 0, 0.048466F, -0.000376F, 0.998825F, 0 });
        data.push_back({ 0.088989F, -0.995196F, 0.04083F, 0, 0.995157F, 0.090554F, 0.038248F, 0, -0.041762F, 0.037228F, 0.998434F, 0 });
        data.push_back({ -0.473616F, -0.880732F, 0, 0, 0.880732F, -0.473616F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ -0.123119F, -0.992392F, 0, 0, 0.992392F, -0.123119F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.159314F, -0.982871F, 0.09265F, 0, 0.983889F, 0.150362F, -0.096712F, 0, 0.081124F, 0.106565F, 0.990991F, 0 });
        data.push_back({ -0.45663F, -0.889657F, 0, 0, 0.889657F, -0.45663F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ -0.076698F, -0.997054F, 0, 0, 0.997054F, -0.076698F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.123006F, -0.978335F, 0.166524F, 0, 0.978335F, 0.091386F, -0.185766F, 0, 0.166524F, 0.185766F, 0.96838F, 0 });
        data.push_back({ -0.366717F, -0.930333F, 0, 0, 0.930333F, -0.366717F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.324171F, -0.945999F, 0, 0, 0.945999F, 0.324171F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.204525F, -0.935955F, 0.286631F, 0, 0.952761F, 0.123178F, -0.277623F, 0, 0.224536F, 0.329871F, 0.916934F, 0 });
        data.push_back({ -0.190355F, -0.981715F, -0.00044F, 0, 0.981715F, -0.190355F, -0.000533F, 0, 0.00044F, -0.000533F, 1, 0 });
        data.push_back({ -0.188246F, -0.982122F, 0, 0, 0.982122F, -0.188246F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.752071F, -0.282712F, -0.595368F, 0, -0.397682F, 0.525706F, -0.751986F, 0, 0.525584F, 0.802314F, 0.282939F, 0 });
        data.push_back({ 0.556184F, -0.830294F, -0.035639F, 0, 0.826703F, 0.557145F, -0.078435F, 0, 0.084981F, 0.014162F, 0.996282F, 0 });
        data.push_back({ 0.620726F, -0.783447F, 0.030166F, 0, 0.782545F, 0.621458F, 0.037589F, 0, -0.048196F, 0.000274F, 0.998838F, 0 });
        data.push_back({ 0.38695F, -0.915355F, -0.111332F, 0, 0.917694F, 0.394073F, -0.050434F, 0, 0.090038F, -0.082654F, 0.992503F, 0 });
        data.push_back({ -0.152033F, -0.988376F, 0, 0, 0.988376F, -0.152033F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.397566F, -0.917574F, 0, 0, 0.917574F, 0.397566F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.076671F, -0.99201F, -0.100188F, 0, 0.996805F, 0.078521F, -0.014653F, 0, 0.022403F, -0.098745F, 0.994861F, 0 });
        data.push_back({ -0.068391F, -0.997659F, 0, 0, 0.997659F, -0.068391F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ -0.050058F, -0.998746F, 0, 0, 0.998746F, -0.050058F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.068248F, -0.982702F, -0.172158F, 0, 0.997656F, 0.068079F, 0.006893F, 0, 0.004947F, -0.172225F, 0.985045F, 0 });
        data.push_back({ 0.093539F, -0.995616F, 0, 0, 0.995616F, 0.093539F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ -0.33522F, -0.94214F, 0, 0, 0.94214F, -0.33522F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.257096F, -0.93156F, -0.257096F, 0, 0.955995F, 0.206258F, 0.208641F, 0, -0.141334F, -0.299423F, 0.943595F, 0 });
        data.push_back({ -0.21201F, -0.977267F, -0.000434F, 0, 0.977267F, -0.21201F, -0.000538F, 0, 0.000434F, -0.000538F, 1, 0 });
        data.push_back({ -0.276492F, -0.961017F, 0, 0, 0.961017F, -0.276492F, 0, 0, 0, 0, 1, 0 });

        copyDataIntoHand(data, handClosed);

        data.erase(data.begin(), data.end());

        data.push_back({ 0.617716F, -0.400404F, 0.676834F, 0, -0.65398F, 0.216427F, 0.724893F, 0, -0.436735F, -0.890414F, -0.128165F, 0 });
        data.push_back({ 0.899514F, -std::numbers::log10e_v<float>, -0.048362F, 0, 0.435479F, 0.89999F, 0.019389F, 0, 0.035107F, -0.038501F, 0.998642F, 0 });
        data.push_back({ 0.945701F, -0.321798F, -0.045777F, 0, 0.321435F, 0.946808F, -0.015267F, 0, 0.048255F, -0.000276F, 0.998835F, 0 });
        data.push_back({ 0.990258F, -0.114774F, 0.078839F, 0, 0.111225F, 0.992634F, 0.048027F, 0, -0.08377F, -0.03879F, 0.99573F, 0 });
        data.push_back({ 0.958294F, -0.285783F, 0, 0, 0.285783F, 0.958294F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.992354F, -0.123425F, 0, 0, 0.123425F, 0.992354F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.951661F, -0.27608F, -0.134618F, 0, 0.266956F, 0.960211F, -0.082032F, 0, 0.151909F, 0.04213F, 0.987496F, 0 });
        data.push_back({ 0.902528F, -0.430632F, -0.000153F, 0, 0.430632F, 0.902527F, 0.000674F, 0, -0.000153F, -0.000674F, 1, 0 });
        data.push_back({ 0.953147F, -0.302508F, 0.000106F, 0, 0.302508F, 0.953147F, -0.000683F, 0, 0.000106F, 0.000683F, 1, 0 });
        data.push_back({ 0.919043F, -0.392269F, -0.038525F, 0, 0.384414F, 0.913631F, -0.132302F, 0, 0.087095F, 0.106782F, 0.990461F, 0 });
        data.push_back({ 0.927023F, -0.375003F, 0, 0, 0.375003F, 0.927023F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.984968F, -0.172734F, 0, 0, 0.172734F, 0.984968F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.825976F, -0.557004F, -0.086665F, 0, 0.534941F, 0.822993F, -0.191102F, 0, 0.17777F, 0.111485F, 0.977737F, 0 });
        data.push_back({ 0.935958F, -0.352111F, 0, 0, 0.352111F, 0.935958F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.833619F, -0.552339F, 0, 0, 0.552339F, 0.833619F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.584889F, -0.400611F, -0.705277F, 0, -0.656401F, 0.277021F, -0.70171F, 0, 0.47649F, 0.873367F, -0.100935F, 0 });
        data.push_back({ 0.812239F, -0.583324F, 0, 0, 0.583324F, 0.812239F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.970436F, -0.241361F, 0, 0, 0.241361F, 0.970436F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.969328F, -0.20464F, -0.136108F, 0, 0.195507F, 0.977633F, -0.077531F, 0, 0.148929F, 0.048543F, 0.987656F, 0 });
        data.push_back({ 0.949484F, -0.313814F, 0, 0, 0.313814F, 0.949484F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.980211F, -0.197957F, 0, 0, 0.197957F, 0.980211F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.954206F, -0.298892F, 0.01245F, 0, 0.29779F, 0.953005F, 0.055697F, 0, -0.028512F, -0.049439F, 0.99837F, 0 });
        data.push_back({ 0.903441F, -0.428712F, 0, 0, 0.428712F, 0.903441F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.967689F, -0.252149F, 0, 0, 0.252149F, 0.967689F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.926338F, -0.376682F, -0.002837F, 0, 0.37216F, 0.914003F, 0.161543F, 0, -0.058257F, -0.1507F, 0.986862F, 0 });
        data.push_back({ 0.914348F, -0.40493F, 0, 0, 0.40493F, 0.914348F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.919149F, -0.39391F, 0, 0, 0.39391F, 0.919149F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.921646F, -0.376617F, 0.09343F, 0, 0.345979F, 0.906603F, 0.241599F, 0, -0.175694F, -0.190344F, 0.965868F, 0 });
        data.push_back({ 0.957083F, -0.289814F, 0, 0, 0.289814F, 0.957083F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.758452F, -0.651728F, 0, 0, 0.651728F, 0.758452F, 0, 0, 0, 0, 1, 0 });

        copyDataIntoHand(data, handOpen);

        if (inPowerArmor) {
            handOpen["LArm_Finger11"].translate = RE::NiPoint3(3.993323F, -4.156268F, 3.585619F);
            handOpen["LArm_Finger12"].translate = RE::NiPoint3(2.893830F, 0.000042F, 0.000004F);
            handOpen["LArm_Finger13"].translate = RE::NiPoint3(4.687409F, 0, 0);
            handOpen["LArm_Finger21"].translate = RE::NiPoint3(8.474635F, -2.161191F, 3.789806F);
            handOpen["LArm_Finger22"].translate = RE::NiPoint3(2.613208F, 0.000026F, 0.000011F);
            handOpen["LArm_Finger23"].translate = RE::NiPoint3(5.145684F, 0, 0);
            handOpen["LArm_Finger31"].translate = RE::NiPoint3(8.151892F, -2.576661F, 1.100114F);
            handOpen["LArm_Finger32"].translate = RE::NiPoint3(3.722714F, 0.000021F, -0.000004F);
            handOpen["LArm_Finger33"].translate = RE::NiPoint3(4.984375F, 0, 0);
            handOpen["LArm_Finger41"].translate = RE::NiPoint3(7.967844F, -2.258833F, -1.337387F);
            handOpen["LArm_Finger42"].translate = RE::NiPoint3(2.933939F, 0.000027F, 0.000004F);
            handOpen["LArm_Finger43"].translate = RE::NiPoint3(5.102559F, 0, 0);
            handOpen["LArm_Finger51"].translate = RE::NiPoint3(8.365221F, -2.603350F, -3.706458F);
            handOpen["LArm_Finger52"].translate = RE::NiPoint3(2.128304F, 0.000018F, 0.000003F);
            handOpen["LArm_Finger53"].translate = RE::NiPoint3(4.594295F, 0, 0);
            handOpen["RArm_Finger11"].translate = RE::NiPoint3(3.993090F, -4.156340F, -3.585553F);
            handOpen["RArm_Finger12"].translate = RE::NiPoint3(2.893783F, 0.000042F, 0.000004F);
            handOpen["RArm_Finger13"].translate = RE::NiPoint3(4.686954F, 0, 0);
            handOpen["RArm_Finger21"].translate = RE::NiPoint3(8.474229F, -2.161169F, -3.789712F);
            handOpen["RArm_Finger22"].translate = RE::NiPoint3(2.613165F, 0.000026F, 0.000011F);
            handOpen["RArm_Finger23"].translate = RE::NiPoint3(5.145271F, 0, 0);
            handOpen["RArm_Finger31"].translate = RE::NiPoint3(8.151529F, -2.576689F, -1.100008F);
            handOpen["RArm_Finger32"].translate = RE::NiPoint3(3.722677F, 0.000021F, -0.000004F);
            handOpen["RArm_Finger33"].translate = RE::NiPoint3(4.973974F, 0, 0);
            handOpen["RArm_Finger41"].translate = RE::NiPoint3(7.967505F, -2.258873F, 1.337498F);
            handOpen["RArm_Finger42"].translate = RE::NiPoint3(2.933841F, 0.000027F, 0.000004F);
            handOpen["RArm_Finger43"].translate = RE::NiPoint3(5.102017F, 0, 0);
            handOpen["RArm_Finger51"].translate = RE::NiPoint3(8.364894F, -2.603419F, 3.706582F);
            handOpen["RArm_Finger52"].translate = RE::NiPoint3(2.128275F, 0.000018F, 0.000003F);
            handOpen["RArm_Finger53"].translate = RE::NiPoint3(4.593989F, 0, 0);
        } else {
            handOpen["LArm_Finger11"].translate = RE::NiPoint3(1.582972F, -1.262648F, 1.853201F);
            handOpen["LArm_Finger12"].translate = RE::NiPoint3(3.569515F, 0.000042F, 0.000004F);
            handOpen["LArm_Finger13"].translate = RE::NiPoint3(2.401824F, 0, 0);
            handOpen["LArm_Finger21"].translate = RE::NiPoint3(7.501364F, 0.430291F, 2.277657F);
            handOpen["LArm_Finger22"].translate = RE::NiPoint3(3.018186F, 0.000026F, 0.000011F);
            handOpen["LArm_Finger23"].translate = RE::NiPoint3(1.850236F, 0, 0);
            handOpen["LArm_Finger31"].translate = RE::NiPoint3(7.595781F, 0.62098F, 0.457392F);
            handOpen["LArm_Finger32"].translate = RE::NiPoint3(3.091653F, 0.000021F, -0.000004F);
            handOpen["LArm_Finger33"].translate = RE::NiPoint3(2.187974F, 0, 0);
            handOpen["LArm_Finger41"].translate = RE::NiPoint3(7.464033F, 0.350152F, -1.438817F);
            handOpen["LArm_Finger42"].translate = RE::NiPoint3(2.664419F, 0.000027F, 0.000004F);
            handOpen["LArm_Finger43"].translate = RE::NiPoint3(1.89974F, 0, 0);
            handOpen["LArm_Finger51"].translate = RE::NiPoint3(6.637259F, -0.35742F, -3.01848F);
            handOpen["LArm_Finger52"].translate = RE::NiPoint3(2.238261F, 0.000018F, 0.000003F);
            handOpen["LArm_Finger53"].translate = RE::NiPoint3(1.665912F, 0, 0);
            handOpen["RArm_Finger11"].translate = RE::NiPoint3(1.582972F, -1.262648F, -1.853201F);
            handOpen["RArm_Finger12"].translate = RE::NiPoint3(3.569515F, 0.000042F, 0.000004F);
            handOpen["RArm_Finger13"].translate = RE::NiPoint3(2.401824F, 0, 0);
            handOpen["RArm_Finger21"].translate = RE::NiPoint3(7.501364F, 0.430291F, -2.277657F);
            handOpen["RArm_Finger22"].translate = RE::NiPoint3(3.018186F, 0.000026F, 0.000011F);
            handOpen["RArm_Finger23"].translate = RE::NiPoint3(1.850236F, 0, 0);
            handOpen["RArm_Finger31"].translate = RE::NiPoint3(7.595781F, 0.62098F, -0.457392F);
            handOpen["RArm_Finger32"].translate = RE::NiPoint3(3.091653F, 0.000021F, -0.000004F);
            handOpen["RArm_Finger33"].translate = RE::NiPoint3(2.187974F, 0, 0);
            handOpen["RArm_Finger41"].translate = RE::NiPoint3(7.464033F, 0.350152F, 1.438817F);
            handOpen["RArm_Finger42"].translate = RE::NiPoint3(2.664419F, 0.000027F, 0.000004F);
            handOpen["RArm_Finger43"].translate = RE::NiPoint3(1.89974F, 0, 0);
            handOpen["RArm_Finger51"].translate = RE::NiPoint3(6.637259F, -0.35742F, 3.01848F);
            handOpen["RArm_Finger52"].translate = RE::NiPoint3(2.238261F, 0.000018F, 0.000003F);
            handOpen["RArm_Finger53"].translate = RE::NiPoint3(1.665912F, 0, 0);
        }
    }

    void setFingerPositionScalar(const bool isLeft, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        const auto* const fingersArray = isLeft ? LEFT_HAND_FINGERS : RIGHT_HAND_FINGERS;
        for (auto i = 0; i < fingersArray->size(); i++) {
            handPapyrusHasControl[fingersArray[i]] = true;
        }

        if (isLeft) {
            handPapyrusPose["LArm_Finger11"] = thumb;
            handPapyrusPose["LArm_Finger12"] = thumb;
            handPapyrusPose["LArm_Finger13"] = thumb;
            handPapyrusPose["LArm_Finger21"] = index;
            handPapyrusPose["LArm_Finger22"] = index;
            handPapyrusPose["LArm_Finger23"] = index;
            handPapyrusPose["LArm_Finger31"] = middle;
            handPapyrusPose["LArm_Finger32"] = middle;
            handPapyrusPose["LArm_Finger33"] = middle;
            handPapyrusPose["LArm_Finger41"] = ring;
            handPapyrusPose["LArm_Finger42"] = ring;
            handPapyrusPose["LArm_Finger43"] = ring;
            handPapyrusPose["LArm_Finger51"] = pinky;
            handPapyrusPose["LArm_Finger52"] = pinky;
            handPapyrusPose["LArm_Finger53"] = pinky;
        } else {
            handPapyrusPose["RArm_Finger11"] = thumb;
            handPapyrusPose["RArm_Finger12"] = thumb;
            handPapyrusPose["RArm_Finger13"] = thumb;
            handPapyrusPose["RArm_Finger21"] = index;
            handPapyrusPose["RArm_Finger22"] = index;
            handPapyrusPose["RArm_Finger23"] = index;
            handPapyrusPose["RArm_Finger31"] = middle;
            handPapyrusPose["RArm_Finger32"] = middle;
            handPapyrusPose["RArm_Finger33"] = middle;
            handPapyrusPose["RArm_Finger41"] = ring;
            handPapyrusPose["RArm_Finger42"] = ring;
            handPapyrusPose["RArm_Finger43"] = ring;
            handPapyrusPose["RArm_Finger51"] = pinky;
            handPapyrusPose["RArm_Finger52"] = pinky;
            handPapyrusPose["RArm_Finger53"] = pinky;
        }
    }

    void restoreFingerPoseControl(const bool isLeft)
    {
        const auto* const fingersArray = isLeft ? LEFT_HAND_FINGERS : RIGHT_HAND_FINGERS;
        for (auto i = 0; i < fingersArray->size(); i++) {
            handPapyrusHasControl[fingersArray[i]] = false;
        }
    }

    void setPipboyHandPose()
    {
        setForceHandPointingPoseExplicitHand(!g_config.leftHandedPipBoy, true);
    }

    void disablePipboyHandPose()
    {
        setForceHandPointingPoseExplicitHand(!g_config.leftHandedPipBoy, false);
    }

    void setConfigModeHandPose()
    {
        setForceHandPointingPose(false, true);
    }

    void disableConfigModePose()
    {
        setForceHandPointingPose(false, false);
    }

    /**
     * Set/Release hand to/from pointing pose for primary hand.
     * Right hand is primary hand if left-handed mode is off, left hand otherwise.
     */
    void setForceHandPointingPose(const bool primaryHand, const bool forcePointing)
    {
        setForceHandPointingPoseExplicitHand(primaryHand ^ f4vr::isLeftHandedMode(), forcePointing);
    }

    /**
     * Set/Release hand to/from pointing pose for explicitly right or left hand.
     */
    void setForceHandPointingPoseExplicitHand(const bool rightHand, const bool override)
    {
        if (_handPointingPoseSet[rightHand] == override) {
            return;
        }
        logger::debug("Set force pointing pose for '{}' hand: {})", rightHand ? "Right" : "Left", override ? "Pointing" : "Release");
        _handPointingPoseSet[rightHand] = override;
        const auto* const fingers = rightHand ? RIGHT_HAND_FINGERS : LEFT_HAND_FINGERS;
        for (auto i = 0; i < FINGERS_COUNT; i++) {
            const std::string finger = fingers[i];
            handPapyrusHasControl[finger.c_str()] = override;
            handPapyrusPose[finger.c_str()] = HAND_FINGERS_POINTING_POSE[i];
        }
    }

    void setOffhandGripHandPose()
    {
        setOffhandGripHandPoseOverride(true);
    }

    void releaseOffhandGripHandPose()
    {
        setOffhandGripHandPoseOverride(false);
    }

    void setOffhandGripHandPoseOverride(const bool override)
    {
        if (_offHandGripPose == override) {
            return;
        }
        logger::debug("Set offhand grip pose override: {})", override ? "Set" : "Release");
        _offHandGripPose = override;
        const auto* const fingers = f4vr::isLeftHandedMode() ? RIGHT_HAND_FINGERS : LEFT_HAND_FINGERS;
        for (auto i = 0; i < FINGERS_COUNT; i++) {
            const std::string finger = fingers[i];
            handPapyrusHasControl[finger.c_str()] = override;
            handPapyrusPose[finger.c_str()] = OFFHAND_FINGERS_GRIP_POSE[i];
        }
    }
}
