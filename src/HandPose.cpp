#include "HandPose.h"
#include "Config.h"

#include <algorithm>
#include <vector>

namespace FRIK {

	std::map<std::string, NiTransform, CaseInsensitiveComparator> handClosed;
	std::map<std::string, NiTransform, CaseInsensitiveComparator> handOpen;

	std::map<std::string, float> handPapyrusPose;
	std::map<std::string, bool> handPapyrusHasControl;

	void initHandPoses(bool inPowerArmor) {
		std::vector<std::vector<float>> data;

		// pulled from the game engine while running idle animations 

		//closed fist first
		data.push_back({ 0.849409,-0.270577,0.453092,0,-0.382631,0.275533,0.881859,0,-0.363453,-0.922426,0.130509,0});
		data.push_back({ 0.698533,-0.713903,0.048938,0,0.710545,0.700093,0.070685,0,-0.084723,-0.014603,0.996297,0});
		data.push_back({ 0.125157,-0.992116,-0.006447,0,0.990953,0.125323,-0.048036,0,0.048466,-0.000376,0.998825,0});
		data.push_back({ 0.088989,-0.995196,0.04083,0,0.995157,0.090554,0.038248,0,-0.041762,0.037228,0.998434,0});
		data.push_back({ -0.473616,-0.880732,0,0,0.880732,-0.473616,0,0,0,0,1,0});
		data.push_back({ -0.123119,-0.992392,0,0,0.992392,-0.123119,0,0,0,0,1,0});
		data.push_back({ 0.159314,-0.982871,0.09265,0,0.983889,0.150362,-0.096712,0,0.081124,0.106565,0.990991,0});
		data.push_back({ -0.45663,-0.889657,0,0,0.889657,-0.45663,0,0,0,0,1,0});
		data.push_back({ -0.076698,-0.997054,0,0,0.997054,-0.076698,0,0,0,0,1,0});
		data.push_back({ 0.123006,-0.978335,0.166524,0,0.978335,0.091386,-0.185766,0,0.166524,0.185766,0.96838,0});
		data.push_back( { -0.366717,-0.930333,0,0,0.930333,-0.366717,0,0,0,0,1,0});
		data.push_back( { 0.324171,-0.945999,0,0,0.945999,0.324171,0,0,0,0,1,0});
		data.push_back( { 0.204525,-0.935955,0.286631,0,0.952761,0.123178,-0.277623,0,0.224536,0.329871,0.916934,0});
		data.push_back( { -0.190355,-0.981715,-0.00044,0,0.981715,-0.190355,-0.000533,0,0.00044,-0.000533,1,0});
		data.push_back( { -0.188246,-0.982122,0,0,0.982122,-0.188246,0,0,0,0,1,0});
		data.push_back( { 0.752071,-0.282712,-0.595368,0,-0.397682,0.525706,-0.751986,0,0.525584,0.802314,0.282939,0});
		data.push_back( { 0.556184,-0.830294,-0.035639,0,0.826703,0.557145,-0.078435,0,0.084981,0.014162,0.996282,0});
		data.push_back( { 0.620726,-0.783447,0.030166,0,0.782545,0.621458,0.037589,0,-0.048196,0.000274,0.998838,0});
		data.push_back( { 0.38695,-0.915355,-0.111332,0,0.917694,0.394073,-0.050434,0,0.090038,-0.082654,0.992503,0});
		data.push_back( { -0.152033,-0.988376,0,0,0.988376,-0.152033,0,0,0,0,1,0});
		data.push_back( { 0.397566,-0.917574,0,0,0.917574,0.397566,0,0,0,0,1,0});
		data.push_back( { 0.076671,-0.99201,-0.100188,0,0.996805,0.078521,-0.014653,0,0.022403,-0.098745,0.994861,0});
		data.push_back( { -0.068391,-0.997659,0,0,0.997659,-0.068391,0,0,0,0,1,0});
		data.push_back( { -0.050058,-0.998746,0,0,0.998746,-0.050058,0,0,0,0,1,0});
		data.push_back( { 0.068248,-0.982702,-0.172158,0,0.997656,0.068079,0.006893,0,0.004947,-0.172225,0.985045,0});
		data.push_back( { 0.093539,-0.995616,0,0,0.995616,0.093539,0,0,0,0,1,0});
		data.push_back( { -0.33522,-0.94214,0,0,0.94214,-0.33522,0,0,0,0,1,0});
		data.push_back( { 0.257096,-0.93156,-0.257096,0,0.955995,0.206258,0.208641,0,-0.141334,-0.299423,0.943595,0});
		data.push_back( { -0.21201,-0.977267,-0.000434,0,0.977267,-0.21201,-0.000538,0,0.000434,-0.000538,1,0});
		data.push_back( { -0.276492,-0.961017,0,0,0.961017,-0.276492,0,0,0,0,1,0});

		std::copy(data[0].begin(), data[0].end(), handClosed["LArm_Finger11"].rot.arr);
		std::copy(data[1].begin(), data[1].end(), handClosed["LArm_Finger12"].rot.arr);
		std::copy(data[2].begin(), data[2].end(), handClosed["LArm_Finger13"].rot.arr);
		std::copy(data[3].begin(), data[3].end(), handClosed["LArm_Finger21"].rot.arr);
		std::copy(data[4].begin(), data[4].end(), handClosed["LArm_Finger22"].rot.arr);
		std::copy(data[5].begin(), data[5].end(), handClosed["LArm_Finger23"].rot.arr);
		std::copy(data[6].begin(), data[6].end(), handClosed["LArm_Finger31"].rot.arr);
		std::copy(data[7].begin(), data[7].end(), handClosed["LArm_Finger32"].rot.arr);
		std::copy(data[8].begin(), data[8].end(), handClosed["LArm_Finger33"].rot.arr);
		std::copy(data[9].begin(), data[9].end(), handClosed["LArm_Finger41"].rot.arr);
		std::copy(data[10].begin(), data[10].end(), handClosed["LArm_Finger42"].rot.arr);
		std::copy(data[11].begin(), data[11].end(), handClosed["LArm_Finger43"].rot.arr);
		std::copy(data[12].begin(), data[12].end(), handClosed["LArm_Finger51"].rot.arr);
		std::copy(data[13].begin(), data[13].end(), handClosed["LArm_Finger52"].rot.arr);
		std::copy(data[14].begin(), data[14].end(), handClosed["LArm_Finger53"].rot.arr);
		std::copy(data[15].begin(), data[15].end(), handClosed["RArm_Finger11"].rot.arr);
		std::copy(data[16].begin(), data[16].end(), handClosed["RArm_Finger12"].rot.arr);
		std::copy(data[17].begin(), data[17].end(), handClosed["RArm_Finger13"].rot.arr);
		std::copy(data[18].begin(), data[18].end(), handClosed["RArm_Finger21"].rot.arr);
		std::copy(data[19].begin(), data[19].end(), handClosed["RArm_Finger22"].rot.arr);
		std::copy(data[20].begin(), data[20].end(), handClosed["RArm_Finger23"].rot.arr);
		std::copy(data[21].begin(), data[21].end(), handClosed["RArm_Finger31"].rot.arr);
		std::copy(data[22].begin(), data[22].end(), handClosed["RArm_Finger32"].rot.arr);
		std::copy(data[23].begin(), data[23].end(), handClosed["RArm_Finger33"].rot.arr);
		std::copy(data[24].begin(), data[24].end(), handClosed["RArm_Finger41"].rot.arr);
		std::copy(data[25].begin(), data[25].end(), handClosed["RArm_Finger42"].rot.arr);
		std::copy(data[26].begin(), data[26].end(), handClosed["RArm_Finger43"].rot.arr);
		std::copy(data[27].begin(), data[27].end(), handClosed["RArm_Finger51"].rot.arr);
		std::copy(data[28].begin(), data[28].end(), handClosed["RArm_Finger52"].rot.arr);
		std::copy(data[29].begin(), data[29].end(), handClosed["RArm_Finger53"].rot.arr);

		data.erase(data.begin(), data.end());

		data.push_back({ 0.617716,-0.400404,0.676834,0,-0.65398,0.216427,0.724893,0,-0.436735,-0.890414,-0.128165,0 });
		data.push_back({ 0.899514,-0.434207,-0.048362,0,0.435479,0.89999,0.019389,0,0.035107,-0.038501,0.998642,0 });
		data.push_back({ 0.945701,-0.321798,-0.045777,0,0.321435,0.946808,-0.015267,0,0.048255,-0.000276,0.998835,0 });
		data.push_back({ 0.990258,-0.114774,0.078839,0,0.111225,0.992634,0.048027,0,-0.08377,-0.03879,0.99573,0 });
		data.push_back({ 0.958294,-0.285783,0,0,0.285783,0.958294,0,0,0,0,1,0 });
		data.push_back({ 0.992354,-0.123425,0,0,0.123425,0.992354,0,0,0,0,1,0 });
		data.push_back({ 0.951661,-0.27608,-0.134618,0,0.266956,0.960211,-0.082032,0,0.151909,0.04213,0.987496,0 });
		data.push_back({ 0.902528,-0.430632,-0.000153,0,0.430632,0.902527,0.000674,0,-0.000153,-0.000674,1,0 });
		data.push_back({ 0.953147,-0.302508,0.000106,0,0.302508,0.953147,-0.000683,0,0.000106,0.000683,1,0 });
		data.push_back({ 0.919043,-0.392269,-0.038525,0,0.384414,0.913631,-0.132302,0,0.087095,0.106782,0.990461,0 });
		data.push_back({ 0.927023,-0.375003,0,0,0.375003,0.927023,0,0,0,0,1,0 });
		data.push_back({ 0.984968,-0.172734,0,0,0.172734,0.984968,0,0,0,0,1,0 });
		data.push_back({ 0.825976,-0.557004,-0.086665,0,0.534941,0.822993,-0.191102,0,0.17777,0.111485,0.977737,0 });
		data.push_back({ 0.935958,-0.352111,0,0,0.352111,0.935958,0,0,0,0,1,0 });
		data.push_back({ 0.833619,-0.552339,0,0,0.552339,0.833619,0,0,0,0,1,0 });
		data.push_back({ 0.584889,-0.400611,-0.705277,0,-0.656401,0.277021,-0.70171,0,0.47649,0.873367,-0.100935,0 });
		data.push_back({ 0.812239,-0.583324,0,0,0.583324,0.812239,0,0,0,0,1,0 });
		data.push_back({ 0.970436,-0.241361,0,0,0.241361,0.970436,0,0,0,0,1,0 });
		data.push_back({ 0.969328,-0.20464,-0.136108,0,0.195507,0.977633,-0.077531,0,0.148929,0.048543,0.987656,0 });
		data.push_back({ 0.949484,-0.313814,0,0,0.313814,0.949484,0,0,0,0,1,0 });
		data.push_back({ 0.980211,-0.197957,0,0,0.197957,0.980211,0,0,0,0,1,0 });
		data.push_back({ 0.954206,-0.298892,0.01245,0,0.29779,0.953005,0.055697,0,-0.028512,-0.049439,0.99837,0 });
		data.push_back({ 0.903441,-0.428712,0,0,0.428712,0.903441,0,0,0,0,1,0 });
		data.push_back({ 0.967689,-0.252149,0,0,0.252149,0.967689,0,0,0,0,1,0 });
		data.push_back({ 0.926338,-0.376682,-0.002837,0,0.37216,0.914003,0.161543,0,-0.058257,-0.1507,0.986862,0 });
		data.push_back({ 0.914348,-0.40493,0,0,0.40493,0.914348,0,0,0,0,1,0 });
		data.push_back({ 0.919149,-0.39391,0,0,0.39391,0.919149,0,0,0,0,1,0 });
		data.push_back({ 0.921646,-0.376617,0.09343,0,0.345979,0.906603,0.241599,0,-0.175694,-0.190344,0.965868,0 });
		data.push_back({ 0.957083,-0.289814,0,0,0.289814,0.957083,0,0,0,0,1,0 });
		data.push_back({ 0.758452,-0.651728,0,0,0.651728,0.758452,0,0,0,0,1,0 });

		std::copy(data[0].begin(), data[0].end(), handOpen["LArm_Finger11"].rot.arr);
		std::copy(data[1].begin(), data[1].end(), handOpen["LArm_Finger12"].rot.arr);
		std::copy(data[2].begin(), data[2].end(), handOpen["LArm_Finger13"].rot.arr);
		std::copy(data[3].begin(), data[3].end(), handOpen["LArm_Finger21"].rot.arr);
		std::copy(data[4].begin(), data[4].end(), handOpen["LArm_Finger22"].rot.arr);
		std::copy(data[5].begin(), data[5].end(), handOpen["LArm_Finger23"].rot.arr);
		std::copy(data[6].begin(), data[6].end(), handOpen["LArm_Finger31"].rot.arr);
		std::copy(data[7].begin(), data[7].end(), handOpen["LArm_Finger32"].rot.arr);
		std::copy(data[8].begin(), data[8].end(), handOpen["LArm_Finger33"].rot.arr);
		std::copy(data[9].begin(), data[9].end(), handOpen["LArm_Finger41"].rot.arr);
		std::copy(data[10].begin(), data[10].end(), handOpen["LArm_Finger42"].rot.arr);
		std::copy(data[11].begin(), data[11].end(), handOpen["LArm_Finger43"].rot.arr);
		std::copy(data[12].begin(), data[12].end(), handOpen["LArm_Finger51"].rot.arr);
		std::copy(data[13].begin(), data[13].end(), handOpen["LArm_Finger52"].rot.arr);
		std::copy(data[14].begin(), data[14].end(), handOpen["LArm_Finger53"].rot.arr);
		std::copy(data[15].begin(), data[15].end(), handOpen["RArm_Finger11"].rot.arr);
		std::copy(data[16].begin(), data[16].end(), handOpen["RArm_Finger12"].rot.arr);
		std::copy(data[17].begin(), data[17].end(), handOpen["RArm_Finger13"].rot.arr);
		std::copy(data[18].begin(), data[18].end(), handOpen["RArm_Finger21"].rot.arr);
		std::copy(data[19].begin(), data[19].end(), handOpen["RArm_Finger22"].rot.arr);
		std::copy(data[20].begin(), data[20].end(), handOpen["RArm_Finger23"].rot.arr);
		std::copy(data[21].begin(), data[21].end(), handOpen["RArm_Finger31"].rot.arr);
		std::copy(data[22].begin(), data[22].end(), handOpen["RArm_Finger32"].rot.arr);
		std::copy(data[23].begin(), data[23].end(), handOpen["RArm_Finger33"].rot.arr);
		std::copy(data[24].begin(), data[24].end(), handOpen["RArm_Finger41"].rot.arr);
		std::copy(data[25].begin(), data[25].end(), handOpen["RArm_Finger42"].rot.arr);
		std::copy(data[26].begin(), data[26].end(), handOpen["RArm_Finger43"].rot.arr);
		std::copy(data[27].begin(), data[27].end(), handOpen["RArm_Finger51"].rot.arr);
		std::copy(data[28].begin(), data[28].end(), handOpen["RArm_Finger52"].rot.arr);
		std::copy(data[29].begin(), data[29].end(), handOpen["RArm_Finger53"].rot.arr);

		if (inPowerArmor) {
			handOpen["LArm_Finger11"].pos = NiPoint3(3.993323, -4.156268, 3.585619);
			handOpen["LArm_Finger12"].pos = NiPoint3(2.893830, 0.000042, 0.000004);
			handOpen["LArm_Finger13"].pos = NiPoint3(4.687409, 0, 0);
			handOpen["LArm_Finger21"].pos = NiPoint3(8.474635, -2.161191, 3.789806);
			handOpen["LArm_Finger22"].pos = NiPoint3(2.613208, 0.000026, 0.000011);
			handOpen["LArm_Finger23"].pos = NiPoint3(5.145684, 0, 0);
			handOpen["LArm_Finger31"].pos = NiPoint3(8.151892, -2.576661, 1.100114);
			handOpen["LArm_Finger32"].pos = NiPoint3(3.722714, 0.000021, -0.000004);
			handOpen["LArm_Finger33"].pos = NiPoint3(4.984375, 0, 0);
			handOpen["LArm_Finger41"].pos = NiPoint3(7.967844, -2.258833, -1.337387);
			handOpen["LArm_Finger42"].pos = NiPoint3(2.933939, 0.000027, 0.000004);
			handOpen["LArm_Finger43"].pos = NiPoint3(5.102559, 0, 0);
			handOpen["LArm_Finger51"].pos = NiPoint3(8.365221, -2.603350, -3.706458);
			handOpen["LArm_Finger52"].pos = NiPoint3(2.128304, 0.000018, 0.000003);
			handOpen["LArm_Finger53"].pos = NiPoint3(4.594295, 0, 0);
			handOpen["RArm_Finger11"].pos = NiPoint3(3.993090, -4.156340, -3.585553);
			handOpen["RArm_Finger12"].pos = NiPoint3(2.893783, 0.000042, 0.000004);
			handOpen["RArm_Finger13"].pos = NiPoint3(4.686954, 0, 0);
			handOpen["RArm_Finger21"].pos = NiPoint3(8.474229, -2.161169, -3.789712);
			handOpen["RArm_Finger22"].pos = NiPoint3(2.613165, 0.000026, 0.000011);
			handOpen["RArm_Finger23"].pos = NiPoint3(5.145271, 0, 0);
			handOpen["RArm_Finger31"].pos = NiPoint3(8.151529, -2.576689, -1.100008);
			handOpen["RArm_Finger32"].pos = NiPoint3(3.722677, 0.000021, -0.000004);
			handOpen["RArm_Finger33"].pos = NiPoint3(4.973974, 0, 0);
			handOpen["RArm_Finger41"].pos = NiPoint3(7.967505, -2.258873, 1.337498);
			handOpen["RArm_Finger42"].pos = NiPoint3(2.933841, 0.000027, 0.000004);
			handOpen["RArm_Finger43"].pos = NiPoint3(5.102017, 0, 0);
			handOpen["RArm_Finger51"].pos = NiPoint3(8.364894, -2.603419, 3.706582);
			handOpen["RArm_Finger52"].pos = NiPoint3(2.128275, 0.000018, 0.000003);
			handOpen["RArm_Finger53"].pos = NiPoint3(4.593989, 0, 0);
		}
		else {
			handOpen["LArm_Finger11"].pos = NiPoint3(1.582972, -1.262648, 1.853201);
			handOpen["LArm_Finger12"].pos = NiPoint3(3.569515, 0.000042, 0.000004);
			handOpen["LArm_Finger13"].pos = NiPoint3(2.401824, 0, 0);
			handOpen["LArm_Finger21"].pos = NiPoint3(7.501364, 0.430291, 2.277657);
			handOpen["LArm_Finger22"].pos = NiPoint3(3.018186, 0.000026, 0.000011);
			handOpen["LArm_Finger23"].pos = NiPoint3(1.850236, 0, 0);
			handOpen["LArm_Finger31"].pos = NiPoint3(7.595781, 0.62098, 0.457392);
			handOpen["LArm_Finger32"].pos = NiPoint3(3.091653, 0.000021, -0.000004);
			handOpen["LArm_Finger33"].pos = NiPoint3(2.187974, 0, 0);
			handOpen["LArm_Finger41"].pos = NiPoint3(7.464033, 0.350152, -1.438817);
			handOpen["LArm_Finger42"].pos = NiPoint3(2.664419, 0.000027, 0.000004);
			handOpen["LArm_Finger43"].pos = NiPoint3(1.89974, 0, 0);
			handOpen["LArm_Finger51"].pos = NiPoint3(6.637259, -0.35742, -3.01848);
			handOpen["LArm_Finger52"].pos = NiPoint3(2.238261, 0.000018, 0.000003);
			handOpen["LArm_Finger53"].pos = NiPoint3(1.665912, 0, 0);
			handOpen["RArm_Finger11"].pos = NiPoint3(1.582972, -1.262648, -1.853201);
			handOpen["RArm_Finger12"].pos = NiPoint3(3.569515, 0.000042, 0.000004);
			handOpen["RArm_Finger13"].pos = NiPoint3(2.401824, 0, 0);
			handOpen["RArm_Finger21"].pos = NiPoint3(7.501364, 0.430291, -2.277657);
			handOpen["RArm_Finger22"].pos = NiPoint3(3.018186, 0.000026, 0.000011);
			handOpen["RArm_Finger23"].pos = NiPoint3(1.850236, 0, 0);
			handOpen["RArm_Finger31"].pos = NiPoint3(7.595781, 0.62098, -0.457392);
			handOpen["RArm_Finger32"].pos = NiPoint3(3.091653, 0.000021, -0.000004);
			handOpen["RArm_Finger33"].pos = NiPoint3(2.187974, 0, 0);
			handOpen["RArm_Finger41"].pos = NiPoint3(7.464033, 0.350152, 1.438817);
			handOpen["RArm_Finger42"].pos = NiPoint3(2.664419, 0.000027, 0.000004);
			handOpen["RArm_Finger43"].pos = NiPoint3(1.89974, 0, 0);
			handOpen["RArm_Finger51"].pos = NiPoint3(6.637259, -0.35742, 3.01848);
			handOpen["RArm_Finger52"].pos = NiPoint3(2.238261, 0.000018, 0.000003);
			handOpen["RArm_Finger53"].pos = NiPoint3(1.665912, 0, 0);
		}
	}


	void setFingerPositionScalar(bool isLeft, float thumb, float index, float middle, float ring, float pinky) {
		if (isLeft) {
			handPapyrusHasControl["LArm_Finger11"] = true;
			handPapyrusHasControl["LArm_Finger12"] = true;
			handPapyrusHasControl["LArm_Finger13"] = true;
			handPapyrusHasControl["LArm_Finger21"] = true;
			handPapyrusHasControl["LArm_Finger22"] = true;
			handPapyrusHasControl["LArm_Finger23"] = true;
			handPapyrusHasControl["LArm_Finger31"] = true;
			handPapyrusHasControl["LArm_Finger32"] = true;
			handPapyrusHasControl["LArm_Finger33"] = true;
			handPapyrusHasControl["LArm_Finger41"] = true;
			handPapyrusHasControl["LArm_Finger42"] = true;
			handPapyrusHasControl["LArm_Finger43"] = true;
			handPapyrusHasControl["LArm_Finger51"] = true;
			handPapyrusHasControl["LArm_Finger52"] = true;
			handPapyrusHasControl["LArm_Finger53"] = true;
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
		}
		else {
			handPapyrusHasControl["RArm_Finger11"] = true;
			handPapyrusHasControl["RArm_Finger12"] = true;
			handPapyrusHasControl["RArm_Finger13"] = true;
			handPapyrusHasControl["RArm_Finger21"] = true;
			handPapyrusHasControl["RArm_Finger22"] = true;
			handPapyrusHasControl["RArm_Finger23"] = true;
			handPapyrusHasControl["RArm_Finger31"] = true;
			handPapyrusHasControl["RArm_Finger32"] = true;
			handPapyrusHasControl["RArm_Finger33"] = true;
			handPapyrusHasControl["RArm_Finger41"] = true;
			handPapyrusHasControl["RArm_Finger42"] = true;
			handPapyrusHasControl["RArm_Finger43"] = true;
			handPapyrusHasControl["RArm_Finger51"] = true;
			handPapyrusHasControl["RArm_Finger52"] = true;
			handPapyrusHasControl["RArm_Finger53"] = true;
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

	void restoreFingerPoseControl(bool isLeft) {
		if (isLeft) {
			handPapyrusHasControl["LArm_Finger11"] = false;
			handPapyrusHasControl["LArm_Finger12"] = false;
			handPapyrusHasControl["LArm_Finger13"] = false;
			handPapyrusHasControl["LArm_Finger21"] = false;
			handPapyrusHasControl["LArm_Finger22"] = false;
			handPapyrusHasControl["LArm_Finger23"] = false;
			handPapyrusHasControl["LArm_Finger31"] = false;
			handPapyrusHasControl["LArm_Finger32"] = false;
			handPapyrusHasControl["LArm_Finger33"] = false;
			handPapyrusHasControl["LArm_Finger41"] = false;
			handPapyrusHasControl["LArm_Finger42"] = false;
			handPapyrusHasControl["LArm_Finger43"] = false;
			handPapyrusHasControl["LArm_Finger51"] = false;
			handPapyrusHasControl["LArm_Finger52"] = false;
			handPapyrusHasControl["LArm_Finger53"] = false;
		}
		else {
			handPapyrusHasControl["RArm_Finger11"] = false;
			handPapyrusHasControl["RArm_Finger12"] = false;
			handPapyrusHasControl["RArm_Finger13"] = false;
			handPapyrusHasControl["RArm_Finger21"] = false;
			handPapyrusHasControl["RArm_Finger22"] = false;
			handPapyrusHasControl["RArm_Finger23"] = false;
			handPapyrusHasControl["RArm_Finger31"] = false;
			handPapyrusHasControl["RArm_Finger32"] = false;
			handPapyrusHasControl["RArm_Finger33"] = false;
			handPapyrusHasControl["RArm_Finger41"] = false;
			handPapyrusHasControl["RArm_Finger42"] = false;
			handPapyrusHasControl["RArm_Finger43"] = false;
			handPapyrusHasControl["RArm_Finger51"] = false;
			handPapyrusHasControl["RArm_Finger52"] = false;
			handPapyrusHasControl["RArm_Finger53"] = false;
		}
	}

	void setPipboyHandPose() {
		setForceHandPointingPoseExplicitHand(!g_config->leftHandedPipBoy, true);
	}

	void disablePipboyHandPose() {
		setForceHandPointingPoseExplicitHand(!g_config->leftHandedPipBoy, false);
	}

	void setConfigModeHandPose() {
		setForceHandPointingPose(false, true);
	}

	void disableConfigModePose() {
		setForceHandPointingPose(false, false);
	}

	static const std::string LEFT_HAND_FINGERS[15] = {
		"LArm_Finger11", "LArm_Finger12", "LArm_Finger13", "LArm_Finger21", "LArm_Finger22", "LArm_Finger23", "LArm_Finger31", "LArm_Finger32", "LArm_Finger33", "LArm_Finger41",
		"LArm_Finger42", "LArm_Finger43", "LArm_Finger51", "LArm_Finger52", "LArm_Finger53"
	};
	static const std::string RIGHT_HAND_FINGERS[15] = {
		"RArm_Finger11", "RArm_Finger12", "RArm_Finger13", "RArm_Finger21", "RArm_Finger22", "RArm_Finger23", "RArm_Finger31", "RArm_Finger32", "RArm_Finger33", "RArm_Finger41",
		"RArm_Finger42", "RArm_Finger43", "RArm_Finger51", "RArm_Finger52", "RArm_Finger53"
	};
	static constexpr float HAND_FINGERS_POINTING_POSE[15] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	static bool _handPointingPoseSet[2] = {false, false};

	/**
	 * Set/Release hand to/from pointing pose for primary hand.
	 * Right hand is primary hand if left-handed mode is off, left hand otherwise.
	 */
	void setForceHandPointingPose(const bool primaryHand, const bool forcePointing) {
		setForceHandPointingPoseExplicitHand(primaryHand ^ g_config->leftHandedMode, forcePointing);
	}

	/**
	 * Set/Release hand to/from pointing pose for explicitly right or left hand.
	 */
	void setForceHandPointingPoseExplicitHand(const bool rightHand, const bool forcePointing) {
		if (_handPointingPoseSet[rightHand] == forcePointing) {
			return;
		}
		_VMESSAGE("Set force pointing pose for '%s' hand: %s)", rightHand ? "Right" : "Left", forcePointing ? "Pointing" : "Release");
		_handPointingPoseSet[rightHand] = forcePointing;
		const auto fingers = rightHand ? RIGHT_HAND_FINGERS : LEFT_HAND_FINGERS;
		for (int x = 0; x < 15; x++) {
			std::string f = fingers[x];
			handPapyrusHasControl[f.c_str()] = forcePointing;
			handPapyrusPose[f.c_str()] = HAND_FINGERS_POINTING_POSE[x];
		}
	}
}
