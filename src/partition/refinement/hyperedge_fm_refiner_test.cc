/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/datastructure/Hypergraph.h"
#include "lib/definitions.h"
#include "partition/Metrics.h"
#include "partition/refinement/HyperedgeFMRefiner.h"

using::testing::Test;
using::testing::Eq;

using defs::INVALID_PARTITION;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;
using datastructure::HyperedgeWeight;

namespace partition {
class AHyperedgeFMRefiner : public Test {
  public:
  AHyperedgeFMRefiner() :
    hypergraph(nullptr),
    config() { }

  std::unique_ptr<HypergraphType> hypergraph;
  Configuration<HypergraphType> config;
};

TEST_F(AHyperedgeFMRefiner, DetectsNestedHyperedgesViaBitvectorProbing) {
  hypergraph.reset(new HypergraphType(3, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 5 },
                                      HyperedgeVector { 0, 1, 2, 0, 1 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 0);

  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.isPartiallyNestedIntoHyperedge(0, 1, 0), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner.isPartiallyNestedIntoHyperedge(1, 0, 0), Eq(true));
}

TEST_F(AHyperedgeFMRefiner, OnlyConsidersPinsInRelevantPartitionWhenDetectingNestedHyperedges) {
  hypergraph.reset(new HypergraphType(4, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 5 },
                                      HyperedgeVector { 0, 2, 3, 1, 2 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);

  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  ASSERT_THAT(hyperedge_fm_refiner.isPartiallyNestedIntoHyperedge(1, 0, 1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner.isPartiallyNestedIntoHyperedge(1, 0, 0), Eq(false));
}

TEST_F(AHyperedgeFMRefiner, ComputesGainOfMovingAllPinsFromOneToAnotherPartition) {
  hypergraph.reset(new HypergraphType(7, 3, HyperedgeIndexVector { 0, 3, 7, /*sentinel*/ 9 },
                                      HyperedgeVector { 0, 5, 6, 1, 2, 3, 4, 4, 5 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(4, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(5, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(6, INVALID_PARTITION, 1);

  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 0, 1), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 0, 1), Eq(0));
}

TEST_F(AHyperedgeFMRefiner, ComputesGainValuesOnModifiedHypergraph) {
  hypergraph.reset(new HypergraphType(7, 3, HyperedgeIndexVector { 0, 3, 7, /*sentinel*/ 9 },
                                      HyperedgeVector { 0, 5, 6, 1, 2, 3, 4, 4, 5 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(4, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(5, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(6, INVALID_PARTITION, 1);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 0, 1), Eq(0));

  hypergraph->changeNodePartition(5, 1, 0);
  hypergraph->changeNodePartition(6, 1, 0);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(2));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 1, 0), Eq(1));
}

TEST_F(AHyperedgeFMRefiner, ConsidersEachHyperedgeOnlyOnceDuringGainComputation) {
  hypergraph.reset(new HypergraphType(4, 3, HyperedgeIndexVector { 0, 2, 5, /*sentinel*/ 8 },
                                      HyperedgeVector { 0, 1, 1, 2, 3, 0, 2, 3 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(2));
}

TEST_F(AHyperedgeFMRefiner, IncreasesGainOfHyperedgeMovementByOneWhenNestedCutHyperedgesExist) {
  hypergraph.reset(new HypergraphType(6, 3, HyperedgeIndexVector { 0, 6, 9, /*sentinel*/ 11 },
                                      HyperedgeVector { 0, 1, 2, 3, 4, 5, 0, 2, 3, 1, 4 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(4, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(5, INVALID_PARTITION, 1);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(3));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(3));
}

TEST_F(AHyperedgeFMRefiner, DoesNotChangeGainOfHyperedgeMovementForNonNestedCutHyperedges) {
  hypergraph.reset(new HypergraphType(5, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 6 },
                                      HyperedgeVector { 0, 1, 2, 2, 4, 3 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(4, INVALID_PARTITION, 0);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(1));
}

TEST_F(AHyperedgeFMRefiner, DoesNotChangeGainOfHyperedgeMovementForNestedNonCutHyperedges) {
  hypergraph.reset(new HypergraphType(3, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 5 },
                                      HyperedgeVector { 0, 1, 2, 1, 2 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(1));
}

TEST_F(AHyperedgeFMRefiner, DecreasesGainOfHyperedgeMovementByOneWhenNonNestedNonCutHyperedgesExist) {
  hypergraph.reset(new HypergraphType(4, 2, HyperedgeIndexVector { 0, 2, /*sentinel*/ 5 },
                                      HyperedgeVector { 0, 1, 1, 2, 3 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 0);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(0));
}
} // namespace partition