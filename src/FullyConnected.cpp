#include <OpenANN/layers/FullyConnected.h>
#include <OpenANN/util/Random.h>

namespace OpenANN
{

FullyConnected::FullyConnected(OutputInfo info, int J, bool bias,
                               ActivationFunction act, double stdDev,
                               Regularization regularization)
  : I(info.outputs()), J(J), bias(bias), act(act), stdDev(stdDev),
    W(J, I), Wd(J, I), b(J), bd(J), x(0), a(1, J), y(1, J), yd(1, J),
    deltas(1, J), e(1, I), regularization(regularization)
{
}

OutputInfo FullyConnected::initialize(std::vector<double*>& parameterPointers,
                                      std::vector<double*>& parameterDerivativePointers)
{
  parameterPointers.reserve(parameterPointers.size() + J * (I + bias));
  parameterDerivativePointers.reserve(parameterDerivativePointers.size() + J * (I + bias));
  for(int j = 0; j < J; j++)
  {
    for(int i = 0; i < I; i++)
    {
      parameterPointers.push_back(&W(j, i));
      parameterDerivativePointers.push_back(&Wd(j, i));
    }
    if(bias)
    {
      parameterPointers.push_back(&b(j));
      parameterDerivativePointers.push_back(&bd(j));
    }
  }

  initializeParameters();

  OutputInfo info;
  info.dimensions.push_back(J);
  return info;
}

void FullyConnected::initializeParameters()
{
  RandomNumberGenerator rng;
  rng.fillNormalDistribution(W, stdDev);
  if(bias)
    rng.fillNormalDistribution(b, stdDev);
}

void FullyConnected::updatedParameters()
{
  if(regularization.maxSquaredWeightNorm > 0.0)
  {
    for(int j = 0; j < J; j++)
    {
      const double squaredNorm = W.row(j).squaredNorm();
      if(squaredNorm > regularization.maxSquaredWeightNorm)
        W.row(j) *= sqrt(regularization.maxSquaredWeightNorm / squaredNorm);
    }
  }
}

void FullyConnected::forwardPropagate(Eigen::MatrixXd* x, Eigen::MatrixXd*& y,
                                      bool dropout, double* error)
{
  const int N = x->rows();
  this->y.conservativeResize(N, Eigen::NoChange);
  this->x = x;
  // Activate neurons
  a = *x * W.transpose();
  if(bias)
    a.rowwise() += b.transpose();
  // Compute output
  activationFunction(act, a, this->y);
  // Add regularization error
  if(error && regularization.l1Penalty > 0.0)
    *error += regularization.l1Penalty * W.array().abs().sum();
  if(error && regularization.l2Penalty > 0.0)
    *error += regularization.l2Penalty * W.array().square().sum() / 2.0;
  y = &(this->y);
}

void FullyConnected::backpropagate(Eigen::MatrixXd* ein,
                                   Eigen::MatrixXd*& eout,
                                   bool backpropToPrevious)
{
  const int N = a.rows();
  yd.conservativeResize(N, Eigen::NoChange);
  // Derive activations
  activationFunctionDerivative(act, y, yd);
  deltas = yd.cwiseProduct(*ein);
  // Weight derivatives
  Wd = deltas.transpose() * *x;
  if(bias)
    bd = deltas.colwise().sum().transpose();
  if(regularization.l1Penalty > 0.0)
    Wd.array() += regularization.l1Penalty * W.array() / W.array().abs();
  if(regularization.l2Penalty > 0.0)
    Wd += regularization.l2Penalty * W;
  // Prepare error signals for previous layer
  if(backpropToPrevious)
    e = deltas * W;
  eout = &e;
}

Eigen::MatrixXd& FullyConnected::getOutput()
{
  return y;
}

Eigen::VectorXd FullyConnected::getParameters()
{
  Eigen::VectorXd p(J*(I+bias));
  int idx = 0;
  for(int j = 0; j < J; j++)
    for(int i = 0; i < I; i++)
      p(idx++) = W(j, i);
  if(bias)
    for(int j = 0; j < J; j++)
      p(idx++) = b(j);
  return p;
}

}
