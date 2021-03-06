/*
 *  Copyright (c) 2017, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/portability/GMock.h>
#include <gtest/gtest.h>
#include <wangle/ssl/SSLStats.h>
#include <wangle/ssl/TLSTicketKeyManager.h>

using ::testing::InSequence;

class MockSSLStats : public wangle::SSLStats {
 public:
  MOCK_QUALIFIED_METHOD1(recordTLSTicketRotation, noexcept, void(bool valid));

  // downstream
  void recordSSLAcceptLatency(int64_t /* unused */) noexcept override {}
  void recordTLSTicket(bool /* unused */, bool /* unused */) noexcept override {
  }
  void recordSSLSession(
      bool /* unused */,
      bool /* unused */,
      bool /* unused */) noexcept override {}
  void recordSSLSessionRemove() noexcept override {}
  void recordSSLSessionFree(uint32_t /* unused */) noexcept override {}
  void recordSSLSessionSetError(uint32_t /* unused */) noexcept override {}
  void recordSSLSessionGetError(uint32_t /* unused */) noexcept override {}
  void recordClientRenegotiation() noexcept override {}
  void recordSSLClientCertificateMismatch() noexcept override {}

  // upstream
  void recordSSLUpstreamConnection(bool /* unused */) noexcept override {}
  void recordSSLUpstreamConnectionError(bool /* unused */) noexcept override {}
};

TEST(TLSTicketKeyManager, TestSetGetTLSTicketKeySeeds) {
  std::vector<std::string> origOld = {"67"};
  std::vector<std::string> origCurr = {"68"};
  std::vector<std::string> origNext = {"69"};

  folly::SSLContext ctx;
  wangle::TLSTicketKeyManager manager(&ctx, nullptr);

  manager.setTLSTicketKeySeeds(origOld, origCurr, origNext);
  std::vector<std::string> old;
  std::vector<std::string> curr;
  std::vector<std::string> next;
  manager.getTLSTicketKeySeeds(old, curr, next);
  ASSERT_EQ(origOld, old);
  ASSERT_EQ(origCurr, curr);
  ASSERT_EQ(origNext, next);
}

TEST(TLSTicketKeyManager, TestValidateTicketSeedsSuccess) {
  MockSSLStats stats;
  EXPECT_CALL(stats, recordTLSTicketRotation(true)).Times(2);

  std::vector<std::string> origOld = {"67", "77"};
  std::vector<std::string> origCurr = {"68", "78"};
  std::vector<std::string> origNext = {"69", "79"};

  // The new ticket seeds are compatible
  std::vector<std::string> newOld = {"68", "78"};
  std::vector<std::string> newCurr = {"69", "79"};
  std::vector<std::string> newNext = {"70", "80"};

  folly::SSLContext ctx;
  wangle::TLSTicketKeyManager manager(&ctx, &stats);

  manager.setTLSTicketKeySeeds(origOld, origCurr, origNext);
  manager.setTLSTicketKeySeeds(newOld, newCurr, newNext);
}

TEST(TLSTicketKeyManager, TestValidateTicketSeedsIdempotent) {
  MockSSLStats stats;
  EXPECT_CALL(stats, recordTLSTicketRotation(true)).Times(2);

  std::vector<std::string> origOld = {"67", "77"};
  std::vector<std::string> origCurr = {"68", "78"};
  std::vector<std::string> origNext = {"69", "79"};

  folly::SSLContext ctx;
  wangle::TLSTicketKeyManager manager(&ctx, &stats);

  manager.setTLSTicketKeySeeds(origOld, origCurr, origNext);
  manager.setTLSTicketKeySeeds(origOld, origCurr, origNext);
}

TEST(TLSTicketKeyManager, TestValidateTicketSeedsFailure) {
  MockSSLStats stats;
  InSequence inSequence;
  EXPECT_CALL(stats, recordTLSTicketRotation(true)).Times(1);
  EXPECT_CALL(stats, recordTLSTicketRotation(false)).Times(1);

  std::vector<std::string> origOld = {"67", "77"};
  std::vector<std::string> origCurr = {"68", "78"};
  std::vector<std::string> origNext = {"69", "79"};

  // The new seeds are incompatible
  std::vector<std::string> newOld = {"69", "79"};
  std::vector<std::string> newCurr = {"70", "80"};
  std::vector<std::string> newNext = {"71", "81"};

  folly::SSLContext ctx;
  wangle::TLSTicketKeyManager manager(&ctx, &stats);

  manager.setTLSTicketKeySeeds(origOld, origCurr, origNext);
  manager.setTLSTicketKeySeeds(newOld, newCurr, newNext);
}

TEST(TLSTicketKeyManager, TestValidateTicketSeedsSubsetPass) {
  MockSSLStats stats;
  InSequence inSequence;
  EXPECT_CALL(stats, recordTLSTicketRotation(true)).Times(2);

  std::vector<std::string> origOld = {"67"};
  std::vector<std::string> origCurr = {"68"};
  std::vector<std::string> origNext = {"69"};

  // The new ticket seeds are compatible
  std::vector<std::string> newOld = {"68", "78"};
  std::vector<std::string> newCurr = {"69"};
  std::vector<std::string> newNext = {"70", "80"};

  folly::SSLContext ctx;
  wangle::TLSTicketKeyManager manager(&ctx, &stats);

  manager.setTLSTicketKeySeeds(origOld, origCurr, origNext);
  manager.setTLSTicketKeySeeds(newOld, newCurr, newNext);
}
