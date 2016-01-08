#include "GeometryFactory.h"

const std::shared_ptr<QuadStripFltIdx> &GeometryFactory::GetQuadStripIdx(ID3D11Device *dev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->quadStripIdx) {
		this->quadStripIdx = std::make_shared<QuadStripFltIdx>(dev);
	}

	return this->quadStripIdx;
}