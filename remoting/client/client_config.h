// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_CLIENT_CONFIG_H_
#define REMOTING_CLIENT_CLIENT_CONFIG_H_

#include <string>

#include "base/basictypes.h"
#include "remoting/protocol/authentication_method.h"

namespace remoting {

struct ClientConfig {
  ClientConfig();
  ~ClientConfig();

  std::string local_jid;

  std::string host_jid;
  std::string host_public_key;

  std::string shared_secret;
  protocol::AuthenticationMethod authentication_method;
  std::string authentication_tag;
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_CLIENT_CONFIG_H_
