#include "utils.h"

#define PI 3.14159265358979323846

namespace F4VRBody {

	float vec3_len(NiPoint3 v1) {

		return sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
	}

	NiPoint3 vec3_norm(NiPoint3 v1) {

		double mag = vec3_len(v1);

		if (mag < 0.000001) {
			float maxX = abs(v1.x);
			float maxY = abs(v1.y);
			float maxZ = abs(v1.z);

			if (maxX >= maxY && maxX >= maxZ) {
				return (v1.x >= 0 ? NiPoint3(1, 0, 0) : NiPoint3(-1, 0, 0));
			}
			else if (maxY > maxZ) {
				return (v1.y >= 0 ? NiPoint3(0, 1, 0) : NiPoint3(0, -1, 0));
			}
			return (v1.z >= 0 ? NiPoint3(0, 0, 1) : NiPoint3(0, 0, -1));

		}
		v1.x /= mag;
		v1.y /= mag;
		v1.z /= mag;

		return v1;
	}

	float vec3_dot(NiPoint3 v1, NiPoint3 v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	NiPoint3 vec3_cross(NiPoint3 v1, NiPoint3 v2) {
		NiPoint3 crossP;

		crossP.x = v1.y * v2.z - v1.z * v2.y;
		crossP.y = v1.z * v2.x - v1.x * v2.z;
		crossP.z = v1.x * v2.y - v1.y * v2.x;

		return crossP;
	}
	
	// the determinant is proportional to the sin of the angle between two vectors.   In 3d case find the sin of the angle between v1 and v2
	// along their angle of rotation with unit vector n
	// https://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors/16544330#16544330
	float vec3_det(NiPoint3 v1, NiPoint3 v2, NiPoint3 n) {
		return (v1.x * v2.y * n.z) + (v2.x * n.y * v1.z) + (n.x * v1.y * v2.z) - (v1.z * v2.y * n.x) - (v2.z * n.y * v1.x) - (n.z * v1.y * v2.x);
	}

	float degrees_to_rads(float deg) {
		return (deg * PI) / 180;
	 }

	float rads_to_degrees(float rad) {
		return (rad * 180) / PI;
	 }


	NiPoint3 rotateXY(NiPoint3 vec, float angle) {
		NiPoint3 retV;

		retV.x = vec.x * cos(angle) - vec.y * sin(angle);
		retV.y = vec.x * sin(angle) + vec.y * cos(angle);
		retV.z = vec.z;

		return retV;
	}

	// Gets a rotation matrix from an axis and an angle
	NiMatrix43 getRotationAxisAngle(NiPoint3 axis, float theta) {
		NiMatrix43 result;
		// This math was found online http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/
		double c = cosf(theta);
		double s = sinf(theta);
		double t = 1.0 - c;
		axis = vec3_norm(axis);
		result.data[0][0] = c + axis.x * axis.x * t;
		result.data[1][1] = c + axis.y * axis.y * t;
		result.data[2][2] = c + axis.z * axis.z * t;
		double tmp1 = axis.x * axis.y * t;
		double tmp2 = axis.z * s;
		result.data[1][0] = tmp1 + tmp2;
		result.data[0][1] = tmp1 - tmp2;
		tmp1 = axis.x * axis.z * t;
		tmp2 = axis.y * s;
		result.data[2][0] = tmp1 - tmp2;
		result.data[0][2] = tmp1 + tmp2;
		tmp1 = axis.y * axis.z * t;
		tmp2 = axis.x * s;
		result.data[2][1] = tmp1 + tmp2;
		result.data[1][2] = tmp1 - tmp2;
		return result.Transpose();
	}

	void updateTransforms(NiNode* node) {
		NiPoint3 pos = node->m_localTransform.pos;
		pos = (node->m_parent->m_worldTransform.rot * (pos * node->m_parent->m_worldTransform.scale));

		node->m_worldTransform.pos = node->m_parent->m_worldTransform.pos + pos;
		
		Matrix44 loc;
		loc.makeTransformMatrix(node->m_localTransform.rot, NiPoint3(0, 0, 0));

		node->m_worldTransform.rot = loc.multiply43Left(node->m_parent->m_worldTransform.rot);

		node->m_worldTransform.scale = node->m_parent->m_worldTransform.scale * node->m_localTransform.scale;
		return;
	 }

}
