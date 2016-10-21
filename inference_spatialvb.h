/*  inference_spatialvb.h - implementation of VB with spatial priors

 Adrian Groves & Michael Chappell, FMRIB Image Analysis Group & IBME QuBIc Group

 Copyright (C) 2007-2015 University of Oxford  */

/*  CCOPYRIGHT */

#include "inference_vb.h"

class CovarianceCache
{
public:

	void CalcDistances(const NEWMAT::Matrix& voxelCoords, const string& distanceMeasure);
	const NEWMAT::SymmetricMatrix& GetDistances() const
	{
		return distances;
	}

	const NEWMAT::ReturnMatrix GetC(double delta) const; // quick to calculate
	const NEWMAT::SymmetricMatrix& GetCinv(double delta) const;

	//  const Matrix& GetCiCodist(double delta) const;
	const NEWMAT::SymmetricMatrix& GetCiCodistCi(double delta, double* CiCodistTrace = NULL) const;

	bool GetCachedInRange(double *guess, double lower, double upper, bool allowEndpoints = false) const;
	// If there's a cached value in (lower, upper), set *guess = value and
	// return true; otherwise return false and don't change *guess.

private:
	NEWMAT::SymmetricMatrix distances;
	typedef map<double, NEWMAT::SymmetricMatrix> Cinv_cache_type;
	mutable Cinv_cache_type Cinv_cache;

	typedef map<double, pair<NEWMAT::SymmetricMatrix, double> > CiCodistCi_cache_type;
	//  mutable CiCodist_cache_type CiCodist_cache; // only really use the Trace
	mutable CiCodistCi_cache_type CiCodistCi_cache;
};

class SpatialVariationalBayes: public VariationalBayesInferenceTechnique
{
public:
	static InferenceTechnique* NewInstance();

	virtual void GetOptions(vector<OptionSpec> &opts) const;

	SpatialVariationalBayes() :
			VariationalBayesInferenceTechnique(), m_spatial_dims(-1)
	{
	}
	virtual void Initialize(FwdModel* fwd_model, FabberRunData& args);
	virtual void DoCalculations(FabberRunData& data);
	//    virtual ~SpatialVariationalBayes();

protected:

	/**
	 * Number of spatial dimensions
	 *
	 * 0 = no spatial smoothing
	 * 1 = Probably not sensible!
	 * 2 = Smoothing in slices only
	 * 3 = Smoothing by volume
	 */
	int m_spatial_dims; // 0 = no spatial norm; 2 = slice only; 3 = volume

	//    bool useDataDrivenSmoothness;
	//    bool useShrinkageMethod;
	//    bool useDirichletBC;
	//    bool useMRF;
	//    bool useMRF2; // without the dirichlet bcs

	/**
	 * Maximum precision increase per iteration
	 */
	double m_spatial_speed; // Should be >1, or -1 = unlimited

	/**
	 * Type of spatial prior to use for each parameter. Should be one
	 * character per parameter, however if string ends with + then
	 * the last character is repeated for remaining parameters
	 */
	std::string m_prior_types_str;

	char m_shrinkage_type;

	/**
	 * Nearest-neighbours of each voxel. Vector size is number of voxels,
	 * each entry is vector of indices of nearest neighbours, starting at 1.
	 *
	 * FIXME Sparse matrix would be better?
	 */
	vector<vector<int> > m_neighbours;

	/**
	 * Next-nearest-neighbours of each voxel. Vector size is number of voxels,
	 * each entry is vector of indices of second nearest neighbours, starting at 1.
	 *
	 * FIXME Sparse matrix would be better?
	 */
	vector<vector<int> > m_neighbours2;

	/**
	 * Calculate first and second nearest neighbours of each voxel
	 */
	void CalcNeighbours(const NEWMAT::Matrix& voxelCoords);

	// For the new (Sahani-based) smoothing method:
	CovarianceCache covar;

	/**
	 * How to measure distances between voxels.
	 *
	 * dist1 = Euclidian distance
	 * dist2 = squared Euclidian distance
	 * mdist = Manhattan distance (|dx| + |dy|)
	 */
	std::string m_dist_measure;

	double fixedDelta;
	double fixedRho;

	/**
	 * Update spatial priors on first iteration?
	 */
	bool m_update_first_iter;

	/**
	 * Use evidence optimization
	 */
	bool m_use_evidence;
	double alwaysInitialDeltaGuess;

	/**
	 * Use full evidence optimization
	 */
	bool m_use_full_evidence;

	/**
	 * Use simultaneous evidence optimization
	 */
	bool m_use_sim_evidence;

	int firstParameterForFullEO;
	bool useCovarianceMarginalsRatherThanPrecisions;
	bool keepInterparameterCovariances;

	int newDeltaEvaluations;

	bool bruteForceDeltaSearch;

	double OptimizeSmoothingScale(const NEWMAT::DiagonalMatrix& covRatio,
	//const SymmetricMatrix& covRatioSupplemented,
			const NEWMAT::ColumnVector& meanDiffRatio, double guess, double* optimizedRho = NULL, bool allowRhoToVary =
					true, bool allowDeltaToVary = true) const;

	double
	OptimizeEvidence(
	// const vector<MVNDist>& fwdPriorVox, // used for parameters other than k
			const vector<MVNDist*>& fwdPosteriorWithoutPrior, // used for parameter k
			// const vector<SymmetricMatrix>& Si,
			int k, const MVNDist* ifp, double guess, bool allowRhoToVary = false, double* rhoOut =
			NULL) const;
};

