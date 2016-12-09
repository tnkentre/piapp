#include "DspCore.h"
#include "vcwrap_cli.h"
#include "dsptop.h"

#define CLASS_NAME        Dsp
#define CLASS_NAME_STR    DSP_NAME_STR
#define FS                DSP_FS
#define FRAMESIZE         DSP_FRAMESIZE
#define CHNUM_IN          DSP_CHNUM_IN
#define CHNUM_OUT         DSP_CHNUM_OUT
#define MON_CPU           1

//----------------------------------------------------------
// declare DSP class
//----------------------------------------------------------
class CLASS_NAME :
  public DspCore
{
public:
  void init();	// to create buffers
  void close();	// to delete buffers
private:
  void execFrame(float** pIn, float** pOut); // to process
  DSPTOPState * dsptop;
};

//----------------------------------------------------------
// DLL requires (Don't edit)
//----------------------------------------------------------
static DspCore* pDspCore;
DspCore* CreateInstance()
{
  if (pDspCore){ return NULL; }
  pDspCore = (DspCore*)(new CLASS_NAME);
  return pDspCore;
}
void ReleaseInstance(DspCore* p)
{
  if (pDspCore && pDspCore == p) {
    delete (CLASS_NAME*)p;
    pDspCore = NULL;
  }
}

//----------------------------------------------------------
// initialize
//----------------------------------------------------------
void CLASS_NAME::init(){
  /* config */
  strcpy_s(m_name, CLASS_NAME_STR);
  m_Fs = FS;
  m_Framesize = FRAMESIZE;
  m_Inputnum = CHNUM_IN;
  m_Outputnum = CHNUM_OUT;
  m_cpu = 0.0f;
#if MON_CPU
  this->addParam("CPU usage", &m_cpu, eDSPCORE_PTYPE_FLT);
#endif

  /* Revo support */
  cli_init(pDspCore);

  /* Initialize DSP */
  dsptop = dsptop_init();

  DspCore::init();
}

//----------------------------------------------------------
// close (Don't edit)
//----------------------------------------------------------
void CLASS_NAME::close(){
  cli_close();
  DspCore::close();
}

//----------------------------------------------------------
// frame processing
//----------------------------------------------------------
void CLASS_NAME::execFrame(float** pIn, float** pOut)
{
  dsptop_proc(dsptop, pOut, pIn);
}
