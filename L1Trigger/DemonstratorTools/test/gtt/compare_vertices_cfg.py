import FWCore.ParameterSet.Config as cms

process = cms.Process("VTXCMP")

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(360)  # set to how many events you want to compare
)

process.source = cms.Source("EmptySource")

process.TFileService = cms.Service(
    "TFileService",
    fileName = cms.string("gttVertexCompare.root")
)

process.vtxCmp = cms.EDAnalyzer(
    "GTTVertexFileComparator",
    referenceFiles = cms.vstring(
        "dataCorrelator/L1GTTOutputToCorrelatorFile_0.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_1.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_2.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_3.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_4.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_5.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_6.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_7.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_8.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_9.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_10.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_11.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_12.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_13.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_14.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_15.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_16.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_17.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_18.txt",
        "dataCorrelator/L1GTTOutputToCorrelatorFile_19.txt",
    ),
    testFiles = cms.vstring(
        "dataOut_fh_nomet_S1/fwout_0_run1.txt",
        "dataOut_fh_nomet_S1/fwout_1_run1.txt",
        "dataOut_fh_nomet_S1/fwout_2_run1.txt",
        "dataOut_fh_nomet_S1/fwout_3_run1.txt",
        "dataOut_fh_nomet_S1/fwout_4_run1.txt",
        "dataOut_fh_nomet_S1/fwout_5_run1.txt",
        "dataOut_fh_nomet_S1/fwout_6_run1.txt",
        "dataOut_fh_nomet_S1/fwout_7_run1.txt",
        "dataOut_fh_nomet_S1/fwout_8_run1.txt",
        "dataOut_fh_nomet_S1/fwout_9_run1.txt",
        "dataOut_fh_nomet_S1/fwout_10_run1.txt",
        "dataOut_fh_nomet_S1/fwout_11_run1.txt",
        "dataOut_fh_nomet_S1/fwout_12_run1.txt",
        "dataOut_fh_nomet_S1/fwout_13_run1.txt",
        "dataOut_fh_nomet_S1/fwout_14_run1.txt",
        "dataOut_fh_nomet_S1/fwout_15_run1.txt",
        "dataOut_fh_nomet_S1/fwout_16_run1.txt",
        "dataOut_fh_nomet_S1/fwout_17_run1.txt",
        "dataOut_fh_nomet_S1/fwout_18_run1.txt",
        "dataOut_fh_nomet_S1/fwout_19_run1.txt",
    ),

    # Link 000 vs Link 088 from your examples
    referenceVertexChannels = cms.vuint32(0),
    testVertexChannels      = cms.vuint32(88),

    format = cms.untracked.string("EMPv2"),
    emptyFramesAtStartRef  = cms.untracked.uint32(0),
    emptyFramesAtStartTest = cms.untracked.uint32(0),
    maxVerticesToCompare   = cms.untracked.uint32(8),
    reverseTestEventTriples = cms.untracked.bool(True),

)

process.p = cms.Path(process.vtxCmp)

