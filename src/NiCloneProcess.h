#pragma once

struct NiCloneProcess {
	UInt64 unk00 = 0;
	UInt64 unk08 = 0; // Start of clone list 1?
	UInt64 unk10 = 0;
	UInt64* unk18; // initd to RelocAddr(0x36ff560)
	UInt64 unk20 = 0;
	UInt64 unk28 = 0;
	UInt64 unk30 = 0;
	UInt64 unk38 = 0; // Start of clone list 2?
	UInt64 unk40 = 0;
	UInt64* unk48; // initd to RelocAddr(0x36ff564)
	UInt64 unk50 = 0;
	UInt64 unk58 = 0;
	UInt8 copyType = 1; // 60 - CopyType - default 1
	UInt8 m_eAffectedNodeRelationBehavior = 0; // 61 - CloneRelationBehavior - default 0
	UInt8 m_eDynamicEffectRelationBehavior = 0; // 62 - CloneRelationBehavior - default 0
	char m_cAppendChar = '$'; // 64 - default '$'
	NiPoint3 scale = {1.0f, 1.0f, 1.0f}; // 0x68 - default {1, 1, 1}
};
