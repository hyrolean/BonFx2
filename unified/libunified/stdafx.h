// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// TODO: reference additional headers your program requires here

#include <Tchar.h>
#include <Windows.h>
#include <Crtdbg.h>

// TODO: �v���O�����ɕK�v�Ȓǉ��w�b�_�[�������ŎQ�Ƃ��Ă��������B

// ���������[�N���o
#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#ifndef __BORLANDC__
#ifdef _MSC_VER
#if _MSC_VER <= 1200 // VC++ 6.0 �ȉ�
//�F�X�Ɩ�肪�����R���p�C���Ȃ̂ŁA�ꍇ�����p�̃t���O��ݒ肵�Ă���
#define __MSVC_1200__
//VC++ for�\���̃C�����C���ϐ������}�N��
#define for if(0) ; else for
#endif
// C4786 �x��������
#pragma warning (disable : 4786)
#endif
#endif

#include "..\dbtrace.h"
#include "..\HRTimer.h"
