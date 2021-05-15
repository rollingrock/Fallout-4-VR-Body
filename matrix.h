#pragma once

#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"

#include "utils.h"

#define PI 3.14159265358979323846

namespace F4VRBody {
	
	class Matrix44 {
	public:
		Matrix44() {
			for (auto i = 0; i < 4; i++) {
				for (auto j = 0; j < 4; j++) {
					data[i][j] = 0.0;
				}
			}
		}

		void makeIdentity() {
			data[0][0] = 1.0;
			data[0][1] = 0.0;
			data[0][2] = 0.0;
			data[0][3] = 0.0;
			data[1][0] = 0.0;
			data[1][1] = 1.0;
			data[1][2] = 0.0;
			data[1][3] = 0.0;
			data[2][0] = 0.0;
			data[2][1] = 0.0;
			data[2][2] = 1.0;
			data[2][3] = 0.0;
			data[3][0] = 0.0;
			data[3][1] = 0.0;
			data[3][2] = 0.0;
			data[3][3] = 0.0;
		}

		void setPosition(float x, float y, float z) {
			data[3][0] = x;
			data[3][1] = y;
			data[3][2] = z;
		}

		void setPosition(NiPoint3 pt) {
			data[3][0] = pt.x;
			data[3][1] = pt.y;
			data[3][2] = pt.z;
		}

		void makeTransformMatrix(NiMatrix43 rot, NiPoint3 pos) {
			for (auto i = 0; i < 3; i++) {
				for (auto j = 0; j < 3; j++) {
					data[i][j] = rot.data[i][j];
				}
			}
			data[0][3] = 0.0;
			data[1][3] = 0.0;
			data[2][3] = 0.0;
			data[3][3] = 1.0;
			data[3][0] = pos.x;
			data[3][1] = pos.y;
			data[3][2] = pos.z;
		}

		NiMatrix43 make43();

		void getEulerAngles(float *heading, float *roll, float *attitude);
		void setEulerAngles(float heading, float roll, float attitude);

		void rotateVectoVec(NiPoint3 toVec, NiPoint3 fromVec);

		NiMatrix43 multiply43Left(NiMatrix43 mat);
		NiMatrix43 multiply43Right(NiMatrix43 mat);
		NiMatrix43 mult(NiMatrix43 left, NiMatrix43 right);

		// Fallout Func
		static void matrixMultiply(Matrix44* worldMat, Matrix44* retMat, Matrix44* localMat);

		//overload
		void operator = (float a_num){
			for (auto i = 0; i < 4; i++) {
				for (auto j = 0; j < 4; j++) {
					data[i][j] = a_num;
				}
			}
		}




		float data[4][4];
	};

}
