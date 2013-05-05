#include "IntrinsicPlasticityTestCase.h"
#include <OpenANN/IntrinsicPlasticity.h>
#include <OpenANN/util/Random.h>
#include <OpenANN/optimization/MBSGD.h>

void IntrinsicPlasticityTestCase::run()
{
  OpenANN::RandomNumberGenerator rng;
  rng.seed(0);
  RUN(IntrinsicPlasticityTestCase, learn);
}

void IntrinsicPlasticityTestCase::learn()
{
  OpenANN::IntrinsicPlasticity ip(2, 0.2);

  OpenANN::RandomNumberGenerator rng;
  int samples = 10000;
  Eigen::MatrixXd X(2, samples);
  Eigen::MatrixXd Y(2, samples);
  for(int i = 0; i < samples; i++)
  {
    for(int j = 0; j < 2; j++)
    {
      X(j, i) = rng.sampleNormalDistribution<double>();
      Y(j, i) = rng.sampleNormalDistribution<double>();
    }
  }
  ip.trainingSet(X, Y);

  ip.initialize();
  Eigen::VectorXd p = ip.currentParameters();
  ASSERT_EQUALS_DELTA(p(0), 1.0, 1e-3);
  ASSERT_EQUALS_DELTA(p(1), 1.0, 1e-3);
  ASSERT_NOT_EQUALS(p(2), 0.0);
  ASSERT_NOT_EQUALS(p(3), 0.0);
  p(2) = 1e-3;
  p(3) = 1e-3;
  ip.setParameters(p);
  p = ip.currentParameters();
  ASSERT_EQUALS_DELTA(p(0), 1.0, 1e-3);
  ASSERT_EQUALS_DELTA(p(1), 1.0, 1e-3);
  ASSERT_NOT_EQUALS(p(2), 0.0);
  ASSERT_NOT_EQUALS(p(3), 0.0);

  Eigen::VectorXd y(2);
  y.fill(0.0);
  for(int i = 0; i < samples; i++)
    y += ip(X.col(i));
  Eigen::VectorXd mean = y / (double) samples;
  ASSERT_EQUALS_DELTA(mean(0), 0.5, 2e-2);
  ASSERT_EQUALS_DELTA(mean(1), 0.5, 2e-2);
  const double e = ip.error();

  OpenANN::MBSGD sgd(5e-5, 0.9, 1);
  OpenANN::StoppingCriteria stop;
  stop.maximalIterations = 1;
  sgd.setOptimizable(ip);
  sgd.setStopCriteria(stop);
  while(sgd.step());

  y.fill(0.0);
  for(int i = 0; i < samples; i++)
    y += ip(X.col(i));
  mean = y / (double) samples;
  ASSERT_EQUALS_DELTA(mean(0), 0.2, 2e-2);
  ASSERT_EQUALS_DELTA(mean(1), 0.2, 2e-2);
  ASSERT(e > ip.error());
}
