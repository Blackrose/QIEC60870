#include "iec101_link_layer_frame.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace QIEC60870::p101;

TEST(LinkLayer, frame_encode_fixedframe) {
  LinkLayerFrame fixedFrame(0x5a, 0x01);
  auto raw = fixedFrame.encode();
  EXPECT_THAT(raw, ElementsAre(0x10, 0x5a, 0x01, 0x5b, 0x16));
}

TEST(LinkLayer, frame_encode_variableframe) {
  LinkLayerFrame variableFrame(
      0x08, 0x01,
      std::vector<uint8_t>(
          {0x46, 0x01, 0x04, 0x01, 0x00, 0x00, 0x00}) /*asdu*/);
  auto raw = variableFrame.encode();
  EXPECT_THAT(raw, ElementsAre(0x68, 0x09, 0x09, 0x68, 0x08, 0x01, 0x46, 0x01,
                               0x04, 0x01, 0x00, 0x00, 0x00, 0x55, 0x16));
}

TEST(LinkLayer, frame_decode_e5_works_well) {
  LinkLayerFrameCodec codec;
  LinkLayerFrame frame;

  codec.decode(std::vector<uint8_t>({0xe5}));
  EXPECT_EQ(codec.error(), FrameParseErr::kNoError);
  frame = codec.toLinkLayerFrame();
  EXPECT_EQ(frame.isSlaveLevel12UserDataEmpty(), true);
}

TEST(LinkLayer, frame_encode_e5_workl_well) {
  LinkLayerFrame frame;
  frame.setSlaveLevel12UserDataIsEmpty();

  auto raw = frame.encode();
  EXPECT_THAT(raw, ElementsAre(0xe5));
}

TEST(LinkLayer, frame_decode_works_well) {
  struct TestCase {
    std::vector<uint8_t> data;
    uint8_t ctrlDomain;
    FrameParseErr error;
    std::string name;
  };
  std::vector<TestCase> cases = {
      {{0x10, 0x5a, 0x01, 0x5b, 0x16}, 0x5a, FrameParseErr::kNoError, "case0"},
      {{0x10, 0x5a, 0x01, 0x5c, 0x16},
       0x5a,
       FrameParseErr::kCheckError,
       "cs check"},
      {{0x40, 0x5a, 0x01, 0x5b, 0x16},
       0x5a,
       FrameParseErr::kBadFormat,
       "0x10 check"},
      {{0x10, 0x5a, 0x01, 0x5b, 0x26},
       0x5a,
       FrameParseErr::kBadFormat,
       "fixed 0x16 check"},
      {{0x10, 0x5a},
       0x5a,
       FrameParseErr::kNeedMoreData,
       "fixed need more data"},
      {{0x68, 0x09, 0x09, 0x68, 0x08, 0x01, 0x46, 0x01, 0x04, 0x01, 0x00, 0x00,
        0x00, 0x55, 0x16},
       0x08,
       FrameParseErr::kNoError,
       "variable success"},
      {{0x68, 0x09, 0x09, 0x99, 0x08, 0x01, 0x46, 0x01, 0x04, 0x01, 0x00, 0x00,
        0x00, 0x55, 0x16},
       0x08,
       FrameParseErr::kBadFormat,
       "second 0x68 check"},
      {{0x68, 0x03, 0x03, 0x68, 0x08, 0x01, 0x46, 0x01, 0x04, 0x01, 0x00, 0x00,
        0x00, 0x55, 0x16},
       0x08,
       FrameParseErr::kBadFormat,
       "lenth check"},
      {{0x68, 0x09, 0x07, 0x68, 0x08, 0x01, 0x46, 0x01, 0x04, 0x01, 0x00, 0x00,
        0x00, 0x55, 0x16},
       0x08,
       FrameParseErr::kCheckError,
       "second length check"},
  };

  for (const auto &test : cases) {
    LinkLayerFrame frame;
    LinkLayerFrameCodec codec;
    codec.decode(test.data);

    EXPECT_EQ(codec.error(), test.error) << test.name;

    if (codec.error() == FrameParseErr::kNoError) {
      frame = codec.toLinkLayerFrame();
      EXPECT_EQ(frame.ctrlDomain(), test.ctrlDomain);
    }
  }
}

TEST(LinkLayer, frameCodec_toLinkLayerFrame_workswell) {
  struct TestCase {
    std::vector<uint8_t> data;
    bool hasAsdu;
    std::string name;
  };

  std::vector<TestCase> cases = {
      {{0x10, 0x5a, 0x01, 0x5b, 0x16}, false, "case0"},
      {{0x68, 0x09, 0x09, 0x68, 0x08, 0x01, 0x46, 0x01, 0x04, 0x01, 0x00, 0x00,
        0x00, 0x55, 0x16},
       true,
       "case5"}};

  for (const auto &test : cases) {
    LinkLayerFrame frame;
    LinkLayerFrameCodec codec;

    codec.decode(test.data);
    EXPECT_EQ(codec.error(), FrameParseErr::kNoError);

    frame = codec.toLinkLayerFrame();
    EXPECT_EQ(frame.hasAsdu(), test.hasAsdu);
  }
}

TEST(LinkLayer, frame_ctrlDomain_check_workswell) {
  LinkLayerFrame frame(0x53 /*0101,0011*/, kInvalidSlaveAddress);

  EXPECT_EQ(frame.isFromStartupStation(), true);
  EXPECT_EQ(frame.isValidFCB(), true);
  EXPECT_EQ(frame.fcb(), false);
  EXPECT_EQ(frame.functionCode(),
            static_cast<int>(StartupFunction::kSendUserData));
  EXPECT_EQ(frame.hasAsdu(), false);
}

TEST(LinkLayer, frame_setPRM) {
  LinkLayerFrame frame;

  frame.setPRM(PRM::kFromStartupStation);
  EXPECT_EQ(frame.isFromStartupStation(), true);

  frame.setPRM(PRM::kFromSlaveStation);
  EXPECT_EQ(frame.isFromStartupStation(), false);
}

TEST(LinkLayer, frame_setDIR) {
  LinkLayerFrame frame;

  frame.setDIR(DIR::kFromMasterStation);
  EXPECT_EQ(frame.isFromMasterStation(), true);

  frame.setDIR(DIR::kFromSlaveStation);
  EXPECT_EQ(frame.isFromMasterStation(), false);
}

TEST(LinkLayer, frame_setFCB) {
  LinkLayerFrame frame;

  frame.setFCB(FCB::k0);
  EXPECT_EQ(frame.fcb(), false);

  frame.setFCB(FCB::k1);
  EXPECT_EQ(frame.fcb(), true);
}

TEST(LinkLayer, frame_setACD) {
  LinkLayerFrame frame;

  frame.setACD(ACD::kLevel1DataWatingAccess);
  EXPECT_EQ(frame.hasLevel1DataWatingAccess(), true);

  frame.setACD(ACD::kLevel1NoDataWatingAccess);
  EXPECT_EQ(frame.hasLevel1DataWatingAccess(), false);
}

TEST(LinkLayer, frame_setFCV) {
  LinkLayerFrame frame;

  frame.setFCV(FCV::kFCBValid);
  EXPECT_EQ(frame.isValidFCB(), true);

  frame.setFCV(FCV::kFCBInvalid);
  EXPECT_EQ(frame.isValidFCB(), false);
}

TEST(LinkLayer, frame_setDFC) {
  LinkLayerFrame frame;

  frame.setDFC(DFC::kSlaveCannotRecv);
  EXPECT_EQ(frame.isSlaveCannotRecv(), true);

  frame.setDFC(DFC::kSlaveCanRecv);
  EXPECT_EQ(frame.isSlaveCannotRecv(), false);
}

TEST(LinkLayer, frame_setFC) {
  LinkLayerFrame frame;

  frame.setFC(static_cast<int>(StartupFunction::kAccessRequest));
  EXPECT_EQ(frame.functionCode(),
            static_cast<int>(StartupFunction::kAccessRequest));
}

TEST(LinkLayer, frame_ctrlDomain_set) {
  LinkLayerFrame frame;

  /// 0111 1000 <===> 0x78
  frame.setPRM(PRM::kFromStartupStation);
  frame.setFCB(FCB::k1);
  frame.setFCV(FCV::kFCBValid);
  frame.setFC(static_cast<int>(StartupFunction::kAccessRequest));

  uint8_t c = frame.ctrlDomain();
  EXPECT_EQ(c, 0x78);
}
