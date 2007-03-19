#ifndef SIMANGLESINC_
#define SIMANGLESINC_

/// Manage the creation of simulated angles. The class is
/// initialized with the simulation parameters. After that,
/// each call to nextAngles() returns the next simulated angle.
class SimAngles(
	public:
		/// Scan types
		enum SIMANGLESMODE {
			PPI, ///< simulate a PPI scan
			RHI	 ///< simulate an RHI scan
		};

		/// Constructor
		SimAngles(SIMANGLESMODE mode,   ///< The simulation mode
			int pulsesPerBeam,			///< The number of pulses per beam, used to compute the per pulse angle increment.
			double beamWidthdegrees,	///< The width of a beam in degrees.
			double azRhiAngle,			///< The azimuth af an RHI simulation
			double elMinAngle,		    ///< The beginning elevation angle
			double elMaxAngle,			///< THe ending elevation angle
			double sweepIncrement,		///< The angle increment of the opposite mode during scan transitions
			int numberPulsesPerTransition ///< The number of pulses in a sweep transition
			);
		/// Destructor
		virtual ~SimAngles();
		/// Compute the next angle information
		void nextAngle(
			double &az, ///< Next azimuth
			double &el, ///< Next elevation
			int &transition, ///< Next transition flag
			int& volume ///< Next volume
			);


	protected:
		/// The current azimuth
		double _az;
		/// The current elevation
		double _el;
		/// the current volume
		int volume;

#endif
