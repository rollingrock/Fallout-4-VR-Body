#pragma once

#include "f4se/NiNodes.h"

#define PI 3.14159265358979323846

namespace FRIK {
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

		void setPosition(const float x, const float y, const float z) {
			data[3][0] = x;
			data[3][1] = y;
			data[3][2] = z;
		}

		void setPosition(const NiPoint3 pt) {
			data[3][0] = pt.x;
			data[3][1] = pt.y;
			data[3][2] = pt.z;
		}

		void makeTransformMatrix(const NiMatrix43& rot, const NiPoint3 pos) {
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

		NiMatrix43 make43() const;

		void getEulerAngles(float* heading, float* roll, float* attitude) const;
		void setEulerAngles(float heading, float roll, float attitude);

		void rotateVectorVec(NiPoint3 toVec, NiPoint3 fromVec);

		NiMatrix43 multiply43Left(const NiMatrix43& mat) const;
		NiMatrix43 multiply43Right(const NiMatrix43& mat) const;
		static NiMatrix43 mult(const NiMatrix43& left, const NiMatrix43& right);

		// Fallout Func
		static void matrixMultiply(const Matrix44* worldMat, const Matrix44* retMat, const Matrix44* localMat);

		//overload
		void operator =(const float a_num) {
			for (auto i = 0; i < 4; i++) {
				for (auto j = 0; j < 4; j++) {
					data[i][j] = a_num;
				}
			}
		}

		float data[4][4];

		static Matrix44 getIdentity() {
			Matrix44 ident;
			ident.makeIdentity();
			return ident;
		}

		static NiMatrix43 getIdentity43() {
			NiMatrix43 ident;
			ident.data[0][0] = 1.0;
			ident.data[0][1] = 0.0;
			ident.data[0][2] = 0.0;
			ident.data[0][3] = 0.0;
			ident.data[1][0] = 0.0;
			ident.data[1][1] = 1.0;
			ident.data[1][2] = 0.0;
			ident.data[1][3] = 0.0;
			ident.data[2][0] = 0.0;
			ident.data[2][1] = 0.0;
			ident.data[2][2] = 1.0;
			ident.data[2][3] = 0.0;
			return ident;
		}
	};
}
