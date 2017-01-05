/*  fwdmodel_trivial.cc - Implements a polynomial model

 Martin Craig

 Copyright (C) 2016 University of Oxford  */

/*  CCOPYRIGHT */

#include "fwdmodel_poly.h"

static int NUM_OPTIONS = 1;
static OptionSpec OPTIONS[] =
{
	{	"degree", OPT_INT, "Maximum power in the polynomial function", OPT_REQ, ""}};

FwdModel* PolynomialFwdModel::NewInstance()
{
	return new PolynomialFwdModel();
}

void PolynomialFwdModel::GetOptions(vector<OptionSpec> &opts) const
{
	for (int i=0; i<NUM_OPTIONS; i++) {
		opts.push_back(OPTIONS[i]);
	}
}

std::string PolynomialFwdModel::GetDescription() const
{
	return "Model which fits data to a simple polynomial function: c0 + c1x + c2x^2 ... etc";
}

string PolynomialFwdModel::ModelVersion() const
{
	return "1.0";
}

void PolynomialFwdModel::Initialize(FabberRunData& args)
{
	FwdModel::Initialize(args);
	m_degree = convertTo<int> (args.GetString("degree"));
}

void PolynomialFwdModel::Evaluate(const NEWMAT::ColumnVector& params, NEWMAT::ColumnVector& result) const
{
	assert(params.Nrows() == m_degree+1);
	result.ReSize(data.Nrows());

	for (int i = 1; i <= data.Nrows(); i++)
	{
		double res = 0;
		int p = 1;
		for (int n = 0; n <= m_degree; n++)
		{
			res += params(n + 1) * p;
			p *= i;
		}
		result(i) = res;
	}
}

int PolynomialFwdModel::NumParams() const
{
	return m_degree + 1;
}

void PolynomialFwdModel::HardcodedInitialDists(MVNDist& prior, MVNDist& posterior) const
{
	// Have to implement this
	assert(prior.means.Nrows() == m_degree+1);
	prior.means = 0;
	prior.SetPrecisions(NEWMAT::IdentityMatrix(m_degree + 1) * 1e-12);
	posterior = prior;
}

void PolynomialFwdModel::NameParams(vector<string>& names) const
{
	names.clear();
	for (int i = 0; i <= m_degree; i++)
	{
		names.push_back((string) "c" + stringify(i));
	}
}

