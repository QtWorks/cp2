/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: FirFilters.h
//
// DESCRIPTION: FIRFILTER Class implementation. This class handles the Fir Filters
//
/////////////////////////////////////////////////////////////////////////////////////////
#define M_PI			3.141592654
#define	SCALE		8191.0

#define export __declspec(dllexport)
class export FIRFILTER
 {
		private:
			unsigned int *m_pPiraq_BaseAddress;
			unsigned int m_iFirRegisterGroup;// Refers to which group of FIR_cfg[][13] is used
			unsigned short	*FIRCH1_I;
			unsigned short	*FIRCH1_Q;
			unsigned short	*FIRCH2_I;
			unsigned short	*FIRCH2_Q;
			long m_lCoeffSum;


		public:
			FIRFILTER( unsigned int *pPiraq_BaseAddress );
			~FIRFILTER();
			void Gaussian(float fFrequencyHz, long lTransmitPulseWidthNs, long lNumCodeBits);
			void FIR_HalfBandWidth(float fFrequencyHz, long lTransmitPulseWidthNs, long lNumCodeBits);   
			void FIR_BoxCar(float fFrequencyHz, long lTransmitPulseWidthNs, long lNumCodeBits);
			double Sinc(double arg);
			void StartFilter(void);
			void ClearFilter( void );
			void ReturnFilterValues(unsigned short *pFIR, unsigned int *piCount );
			
			unsigned short *GetFIRCH1_I(){ return FIRCH1_I;}
			unsigned short *GetFIRCH1_Q(){ return FIRCH1_Q;}
			unsigned short *GetFIRCH2_I(){ return FIRCH2_I;}
			unsigned short *GetFIRCH2_Q(){ return FIRCH2_Q;}

			void SetFIRCH1_I(unsigned short val){ *FIRCH1_I = val;}
			void SetFIRCH1_Q(unsigned short val){ *FIRCH1_Q = val;}
			void SetFIRCH2_I(unsigned short val){ *FIRCH2_I = val;}
			void SetFIRCH2_Q(unsigned short val){ *FIRCH2_Q = val;}			
};
