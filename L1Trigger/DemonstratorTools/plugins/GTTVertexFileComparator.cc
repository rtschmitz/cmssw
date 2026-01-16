// -*- C++ -*-
//
// EDAnalyzer to compare vertex z0 from two EMPv2 file collections
// using BoardDataReader + codecs::decodeVertices, and write ROOT output.
//
// Put in: L1Trigger/DemonstratorTools/plugins/GTTVertexFileComparator.cc
//

#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <algorithm>
#include <array>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"

#include "DataFormats/L1Trigger/interface/VertexWord.h"

#include "L1Trigger/DemonstratorTools/interface/GTTInterface.h"
#include "L1Trigger/DemonstratorTools/interface/BoardDataReader.h"
#include "L1Trigger/DemonstratorTools/interface/codecs/vertices.h"
#include "L1Trigger/DemonstratorTools/interface/utilities.h"

class GTTVertexFileComparator : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit GTTVertexFileComparator(const edm::ParameterSet&);
  ~GTTVertexFileComparator() override = default;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;

  // config
  const l1t::demo::FileFormat format_;
  const std::vector<std::string> referenceFiles_;
  const std::vector<std::string> testFiles_;
  const unsigned int emptyFramesAtStartRef_;
  const unsigned int emptyFramesAtStartTest_;
  const unsigned int maxVerticesToCompare_;

  const std::vector<unsigned int> referenceVertexChannels_;
  const std::vector<unsigned int> testVertexChannels_;

  // If true, reorder the TEST stream in groups of 3 events (swap event 0 <-> 2 within each triple).
  // This matches a common firmware pattern where three GTT blocks output as 2-1-0 instead of 0-1-2.
  const bool reverseTestEventTriples_;

  // readers
  std::optional<l1t::demo::BoardDataReader> refReader_;
  std::optional<l1t::demo::BoardDataReader> testReader_;

  // output
  TH1F* hZ0Ref_{nullptr};
  TH1F* hZ0Test_{nullptr};
  TH1F* hDz0Lead_{nullptr};
  TH1F* hNValidRef_{nullptr};
  TH1F* hNValidTest_{nullptr};

  TTree* tree_{nullptr};

  // tree branches
  ULong64_t iev_{0};
  Int_t nValidRef_{0};
  Int_t nValidTest_{0};
  Float_t z0RefLead_{0.f};
  Float_t z0TestLead_{0.f};
  Float_t dz0Lead_{0.f};
  Int_t hasRefLead_{0};
  Int_t hasTestLead_{0};

  // internal counter
  ULong64_t counter_{0};

  // triple-event buffers for stream realignment
  std::array<l1t::demo::EventData, 3> refBuf_;
  std::array<l1t::demo::EventData, 3> testBuf_;
  unsigned int bufPos_{3};  // 3 => empty / need refill

  static std::optional<l1t::VertexWord> leadingValidVertex(const l1t::VertexWordCollection& vtx) {
    for (const auto& w : vtx) {
      if (w.valid())
        return w;
    }
    return std::nullopt;
  }

  static int countValid(const l1t::VertexWordCollection& vtx) {
    int n = 0;
    for (const auto& w : vtx)
      if (w.valid())
        ++n;
    return n;
  }
};

GTTVertexFileComparator::GTTVertexFileComparator(const edm::ParameterSet& iConfig)
    : format_(l1t::demo::parseFileFormat(iConfig.getUntrackedParameter<std::string>("format"))),
      referenceFiles_(iConfig.getParameter<std::vector<std::string>>("referenceFiles")),
      testFiles_(iConfig.getParameter<std::vector<std::string>>("testFiles")),
      emptyFramesAtStartRef_(iConfig.getUntrackedParameter<unsigned int>("emptyFramesAtStartRef")),
      emptyFramesAtStartTest_(iConfig.getUntrackedParameter<unsigned int>("emptyFramesAtStartTest")),
      maxVerticesToCompare_(iConfig.getUntrackedParameter<unsigned int>("maxVerticesToCompare")),
      referenceVertexChannels_(iConfig.getParameter<std::vector<unsigned int>>("referenceVertexChannels")),
      testVertexChannels_(iConfig.getParameter<std::vector<unsigned int>>("testVertexChannels")),
      reverseTestEventTriples_(iConfig.getUntrackedParameter<bool>("reverseTestEventTriples")) {
  usesResource("TFileService");

  // Build a minimal channel map for {"vertices",0} using the canonical vertex ChannelSpec,
  // but allowing the user to provide the channel indices (e.g. Link 000 vs Link 088).
  const l1t::demo::LinkId verticesId{"vertices", 0};

  const auto& defaultMap = l1t::demo::gtt::kChannelMapOutputToCorrelator;
  const auto it = defaultMap.find(verticesId);
  if (it == defaultMap.end()) {
    throw cms::Exception("GTTVertexFileComparator")
        << "Could not find LinkId{\"vertices\",0} in gtt::kChannelMapOutputToCorrelator";
  }

  const l1t::demo::ChannelSpec spec = it->second.first;

  auto toSizeT = [](const std::vector<unsigned int>& in) {
    std::vector<size_t> out;
    out.reserve(in.size());
    for (auto x : in)
      out.push_back(static_cast<size_t>(x));
    return out;
  };

  l1t::demo::BoardDataReader::ChannelMap_t refChanMap;
  refChanMap.emplace(verticesId, std::make_pair(spec, toSizeT(referenceVertexChannels_)));

  l1t::demo::BoardDataReader::ChannelMap_t testChanMap;
  testChanMap.emplace(verticesId, std::make_pair(spec, toSizeT(testVertexChannels_)));

  // Construct readers
  refReader_.emplace(format_,
                     referenceFiles_,
                     l1t::demo::gtt::kFramesPerTMUXPeriod,
                     l1t::demo::gtt::kGTTBoardTMUX,
                     emptyFramesAtStartRef_,
                     refChanMap);

  testReader_.emplace(format_,
                      testFiles_,
                      l1t::demo::gtt::kFramesPerTMUXPeriod,
                      l1t::demo::gtt::kGTTBoardTMUX,
                      emptyFramesAtStartTest_,
                      testChanMap);
}

void GTTVertexFileComparator::beginJob() {
  edm::Service<TFileService> fs;

  hZ0Ref_      = fs->make<TH1F>("z0_ref", "Input MC z0;z0;Events", 300, -30.0, 30.0);
  hZ0Test_     = fs->make<TH1F>("z0_test", "GTT FW z0;z0;Events", 300, -30.0, 30.0);

  // Keep your original range; you can widen if needed for debugging.
  hDz0Lead_    = fs->make<TH1F>("dz0",
                               "#Deltaz0 (FW - MC);#Deltaz0;Events",
                               300, -15, 15);

  hNValidRef_  = fs->make<TH1F>("nvalid_ref", "N(valid vertices) MC;N;Events", 11, -0.5, 10.5);
  hNValidTest_ = fs->make<TH1F>("nvalid_test", "N(valid vertices) FW;N;Events", 11, -0.5, 10.5);

  tree_ = fs->make<TTree>("events", "Per-event MC-FW vertex z0 comparison");
  tree_->Branch("iev", &iev_, "iev/l");
  tree_->Branch("nValidRef", &nValidRef_, "nValidRef/I");
  tree_->Branch("nValidTest", &nValidTest_, "nValidTest/I");
  tree_->Branch("hasRefLead", &hasRefLead_, "hasRefLead/I");
  tree_->Branch("hasTestLead", &hasTestLead_, "hasTestLead/I");
  tree_->Branch("z0RefLead", &z0RefLead_, "z0RefLead/F");
  tree_->Branch("z0TestLead", &z0TestLead_, "z0TestLead/F");
  tree_->Branch("dz0Lead", &dz0Lead_, "dz0Lead/F");
}

void GTTVertexFileComparator::analyze(const edm::Event&, const edm::EventSetup&) {
  using namespace l1t::demo::codecs;

  // Buffered triple read to realign event ordering between ref and test streams.
  // Each analyze() consumes one element of the currently buffered triple.
  l1t::demo::EventData refEv, testEv;

  if (bufPos_ >= 3) {
    try {
      for (unsigned int i = 0; i < 3; ++i) {
        refBuf_[i]  = refReader_.value().getNextEvent();
        testBuf_[i] = testReader_.value().getNextEvent();
      }
    } catch (const std::exception& e) {
      throw cms::Exception("GTTVertexFileComparator")
          << "Ran out of events in one of the file collections (or decode/read error): " << e.what();
    }

    // If test stream is 2-1-0 per triple, swap 0 and 2 to align with 0-1-2.
    if (reverseTestEventTriples_) {
      std::swap(testBuf_[0], testBuf_[2]);
    }

    bufPos_ = 0;
  }

  refEv  = refBuf_[bufPos_];
  testEv = testBuf_[bufPos_];
  ++bufPos_;

  const l1t::demo::LinkId verticesId{"vertices", 0};

  l1t::VertexWordCollection refVertices  = decodeVertices(refEv.at(verticesId));
  l1t::VertexWordCollection testVertices = decodeVertices(testEv.at(verticesId));

  nValidRef_  = countValid(refVertices);
  nValidTest_ = countValid(testVertices);

  hNValidRef_->Fill(nValidRef_);
  hNValidTest_->Fill(nValidTest_);

  const auto refLead  = leadingValidVertex(refVertices);
  const auto testLead = leadingValidVertex(testVertices);

  hasRefLead_  = refLead.has_value() ? 1 : 0;
  hasTestLead_ = testLead.has_value() ? 1 : 0;

  z0RefLead_  = refLead  ? static_cast<float>(refLead->z0())  : 0.f;
  z0TestLead_ = testLead ? static_cast<float>(testLead->z0()) : 0.f;

  dz0Lead_ = 0.f;
  if (refLead && testLead) {
    dz0Lead_ = z0TestLead_ - z0RefLead_;
    hDz0Lead_->Fill(dz0Lead_);
  }

  if (refLead)  hZ0Ref_->Fill(z0RefLead_);
  if (testLead) hZ0Test_->Fill(z0TestLead_);

  iev_ = counter_++;
  tree_->Fill();

  if ((iev_ % 100) == 0) {
    edm::LogInfo("GTTVertexFileComparator")
        << "Event " << iev_
        << " nValidRef=" << nValidRef_
        << " nValidTest=" << nValidTest_
        << " z0RefLead=" << z0RefLead_
        << " z0TestLead=" << z0TestLead_
        << " dz0Lead=" << dz0Lead_;
  }
}

void GTTVertexFileComparator::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;

  desc.add<std::vector<std::string>>("referenceFiles", {"L1GTTOutputToCorrelatorFile_55.txt"});
  desc.add<std::vector<std::string>>("testFiles", {"fpga_output.txt"});

  // Channel indices to use for {"vertices",0} in each file collection.
  // For your examples: reference is Link 000 -> channel 0, test is Link 088 -> channel 88.
  // If the vertex stream is TMUX’d across multiple channels, provide multiple indices in order.
  desc.add<std::vector<unsigned int>>("referenceVertexChannels", {0});
  desc.add<std::vector<unsigned int>>("testVertexChannels", {88});

  desc.addUntracked<std::string>("format", "EMPv2");
  desc.addUntracked<unsigned int>("emptyFramesAtStartRef", 0);
  desc.addUntracked<unsigned int>("emptyFramesAtStartTest", 0);
  desc.addUntracked<unsigned int>("maxVerticesToCompare", 8);

  // New: set true if firmware outputs events as 2-1-0 within each triple.
  desc.addUntracked<bool>("reverseTestEventTriples", true);

  descriptions.add("GTTVertexFileComparator", desc);
}

DEFINE_FWK_MODULE(GTTVertexFileComparator);

