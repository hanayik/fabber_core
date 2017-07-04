#pragma once

/** 
 * transforms.h
 *
 * Classes defining transformations on model parameters. These may be used
 * as a means of preventing model parameters from violating hard limits, 
 * such as not becoming negative.
 *
 * Copyright (C) 2007-2017 University of Oxford  
 */

/*  CCOPYRIGHT */

#include "rundata.h"

#include <math.h>
#include <string>

const std::string TRANSFORM_CODE_IDENTITY = "I";
const std::string TRANSFORM_CODE_LOG = "L";
const std::string TRANSFORM_CODE_SOFTPLUS = "S";

const char PRIOR_CODE_NORMAL = 'N';
const char PRIOR_CODE_IMAGE = 'I';
const char PRIOR_CODE_ARD = 'A';
const char PRIOR_CODE_SPATIAL_M = 'M';
const char PRIOR_CODE_SPATIAL_m = 'm';
const char PRIOR_CODE_SPATIAL_P = 'P';
const char PRIOR_CODE_SPATIAL_p = 'p';

/**
 * Immutable object describing distribution parameters for single model parameter
 */
struct DistParams 
{
    DistParams(double m=0, double v=1) : m_mean(m), m_var(v), m_prec(1/v) {}
    double mean() const {return m_mean;}
    double var() const {return m_var;}
    double prec() const {return m_prec;}
private:
    double m_mean;
    double m_var;
    double m_prec;
};

/**
 * Abstract base class for parameter transformations.
 */
class Transform
{
public:
    virtual ~Transform() {}

    /**
     * Transform the Fabber internal mean/variance (of the Gaussian
     * distribution) to the mean/variance of the model distribution
     *
     * The default just applies ToModel to the mean and variance
     */
    virtual DistParams ToModel(DistParams params) const;

    /**
     * Transform the mean of the model distribution into the
     * mean of the corresponding Gaussian distribution.
     *
     * The default just applies ToFabber to the mean and variance
     */
    virtual DistParams ToFabber(DistParams params) const;

    /**
     * Transform the Fabber internal value (which is assumed to have a Gaussian
     * distribution) to the value required by the model
     */
    virtual double ToModel(double val) const = 0;

    /**
     * Transform a model parameter value to the value to be used by Fabber internally
     * (and modelled as a Gaussian random variable)
     */
     virtual double ToFabber(double val) const = 0;
};

/**
 * Trivial implementation which performs no transformation
 */
class IdentityTransform : public Transform
{
public:
    double ToModel(double val) const {return val;}
    double ToFabber(double val) const {return val;}
};

/**
 * Transformation for a parameter which is log-normal
 */
class LogTransform : public Transform
{
public:
    virtual DistParams ToModel(DistParams params) const;
    virtual DistParams ToFabber(DistParams params) const;
    double ToModel(double val) const {return exp(val);}
    double ToFabber(double val) const {return log(val);}
};

/**
 * 'Softplus' transformation
 *
 * This is an alternative to the log transform for a parameter which
 * is strictly positive. For positive values it approaches the
 * identity transformation so avoiding any issues associated with the
 * rapid growth of the exponential function.
 */
class SoftPlusTransform : public Transform
{
public:
    double ToModel(double val) const {return log(1+exp(val));}
    double ToFabber(double val) const {return log(exp(val) - 1);}
};

/** Singleton instance of identity transform */
const Transform *TRANSFORM_IDENTITY();

/** Singleton instance of log transform */
const Transform *TRANSFORM_LOG();

/** Singleton instance of softplus transform */
const Transform *TRANSFORM_SOFTPLUS();

const Transform *GetTransform(std::string id);
