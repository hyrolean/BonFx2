// stdafx.h : �W���̃V�X�e�� �C���N���[�h �t�@�C���̃C���N���[�h �t�@�C���A�܂���
// �Q�Ɖ񐔂������A�����܂�ύX����Ȃ��A�v���W�F�N�g��p�̃C���N���[�h �t�@�C��
// ���L�q���܂��B
//

#pragma once

#ifndef _WIN32_WINNT		// Windows XP �ȍ~�̃o�[�W�����ɌŗL�̋@�\�̎g�p�������܂��B
#define _WIN32_WINNT 0x0501	// ����� Windows �̑��̃o�[�W���������ɓK�؂Ȓl�ɕύX���Ă��������B
#endif

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

#include "dbtrace.h"
#include "pryutil.h"
