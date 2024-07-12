#pragma once

#include "MFEMGeneralUserObject.h"
#include "MFEMCoefficient.h"
#include "coefficients.h"

class MFEMMaterial : public MFEMGeneralUserObject
{
public:
  static InputParameters validParams();

  MFEMMaterial(const InputParameters & parameters);
  virtual ~MFEMMaterial();

  virtual void execute() override {}
  virtual void initialize() override {}
  virtual void finalize() override {}

  virtual void storeCoefficients(platypus::Subdomain & subdomain) {}

  std::vector<SubdomainName> blocks;
};
