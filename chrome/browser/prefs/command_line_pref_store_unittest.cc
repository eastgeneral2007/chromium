// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/string_util.h"
#include "base/values.h"
#include "chrome/browser/prefs/command_line_pref_store.h"
#include "chrome/browser/prefs/proxy_config_dictionary.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "ui/base/ui_base_switches.h"

namespace {

const char unknown_bool[] = "unknown_switch";
const char unknown_string[] = "unknown_other_switch";

}  // namespace

class TestCommandLinePrefStore : public CommandLinePrefStore {
 public:
  explicit TestCommandLinePrefStore(CommandLine* cl)
      : CommandLinePrefStore(cl) {}

  bool ProxySwitchesAreValid() {
    return ValidateProxySwitches();
  }

  void VerifyProxyMode(ProxyPrefs::ProxyMode expected_mode) {
    const Value* value = NULL;
    ASSERT_EQ(PrefStore::READ_OK, GetValue(prefs::kProxy, &value));
    ASSERT_EQ(Value::TYPE_DICTIONARY, value->GetType());
    ProxyConfigDictionary dict(static_cast<const DictionaryValue*>(value));
    ProxyPrefs::ProxyMode actual_mode;
    ASSERT_TRUE(dict.GetMode(&actual_mode));
    EXPECT_EQ(expected_mode, actual_mode);
  }

  void VerifySSLCipherSuites(const char* const* ciphers,
                             size_t cipher_count) {
    const Value* value = NULL;
    ASSERT_EQ(PrefStore::READ_OK,
              GetValue(prefs::kCipherSuiteBlacklist, &value));
    ASSERT_EQ(Value::TYPE_LIST, value->GetType());
    const ListValue* list_value = static_cast<const ListValue*>(value);
    ASSERT_EQ(cipher_count, list_value->GetSize());

    std::string cipher_string;
    for (ListValue::const_iterator it = list_value->begin();
         it != list_value->end(); ++it, ++ciphers) {
      ASSERT_TRUE((*it)->GetAsString(&cipher_string));
      EXPECT_EQ(*ciphers, cipher_string);
    }
  }

 private:
  virtual ~TestCommandLinePrefStore() {}
};

// Tests a simple string pref on the command line.
TEST(CommandLinePrefStoreTest, SimpleStringPref) {
  CommandLine cl(CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII(switches::kLang, "hi-MOM");
  scoped_refptr<CommandLinePrefStore> store = new CommandLinePrefStore(&cl);

  const Value* actual = NULL;
  EXPECT_EQ(PrefStore::READ_OK,
            store->GetValue(prefs::kApplicationLocale, &actual));
  std::string result;
  EXPECT_TRUE(actual->GetAsString(&result));
  EXPECT_EQ("hi-MOM", result);
}

// Tests a simple boolean pref on the command line.
TEST(CommandLinePrefStoreTest, SimpleBooleanPref) {
  CommandLine cl(CommandLine::NO_PROGRAM);
  cl.AppendSwitch(switches::kNoProxyServer);
  scoped_refptr<TestCommandLinePrefStore> store =
      new TestCommandLinePrefStore(&cl);

  store->VerifyProxyMode(ProxyPrefs::MODE_DIRECT);
}

// Tests a command line with no recognized prefs.
TEST(CommandLinePrefStoreTest, NoPrefs) {
  CommandLine cl(CommandLine::NO_PROGRAM);
  cl.AppendSwitch(unknown_string);
  cl.AppendSwitchASCII(unknown_bool, "a value");
  scoped_refptr<CommandLinePrefStore> store = new CommandLinePrefStore(&cl);

  const Value* actual = NULL;
  EXPECT_EQ(PrefStore::READ_NO_VALUE, store->GetValue(unknown_bool, &actual));
  EXPECT_EQ(PrefStore::READ_NO_VALUE, store->GetValue(unknown_string, &actual));
}

// Tests a complex command line with multiple known and unknown switches.
TEST(CommandLinePrefStoreTest, MultipleSwitches) {
  CommandLine cl(CommandLine::NO_PROGRAM);
  cl.AppendSwitch(unknown_string);
  cl.AppendSwitchASCII(switches::kProxyServer, "proxy");
  cl.AppendSwitchASCII(switches::kProxyBypassList, "list");
  cl.AppendSwitchASCII(unknown_bool, "a value");
  scoped_refptr<TestCommandLinePrefStore> store =
      new TestCommandLinePrefStore(&cl);

  const Value* actual = NULL;
  EXPECT_EQ(PrefStore::READ_NO_VALUE, store->GetValue(unknown_bool, &actual));
  EXPECT_EQ(PrefStore::READ_NO_VALUE, store->GetValue(unknown_string, &actual));

  store->VerifyProxyMode(ProxyPrefs::MODE_FIXED_SERVERS);

  const Value* value = NULL;
  ASSERT_EQ(PrefStore::READ_OK, store->GetValue(prefs::kProxy, &value));
  ASSERT_EQ(Value::TYPE_DICTIONARY, value->GetType());
  ProxyConfigDictionary dict(static_cast<const DictionaryValue*>(value));

  std::string string_result = "";

  ASSERT_TRUE(dict.GetProxyServer(&string_result));
  EXPECT_EQ("proxy", string_result);

  ASSERT_TRUE(dict.GetBypassList(&string_result));
  EXPECT_EQ("list", string_result);
}

// Tests proxy switch validation.
TEST(CommandLinePrefStoreTest, ProxySwitchValidation) {
  CommandLine cl(CommandLine::NO_PROGRAM);

  // No switches.
  scoped_refptr<TestCommandLinePrefStore> store =
      new TestCommandLinePrefStore(&cl);
  EXPECT_TRUE(store->ProxySwitchesAreValid());

  // Only no-proxy.
  cl.AppendSwitch(switches::kNoProxyServer);
  scoped_refptr<TestCommandLinePrefStore> store2 =
      new TestCommandLinePrefStore(&cl);
  EXPECT_TRUE(store2->ProxySwitchesAreValid());

  // Another proxy switch too.
  cl.AppendSwitch(switches::kProxyAutoDetect);
  scoped_refptr<TestCommandLinePrefStore> store3 =
      new TestCommandLinePrefStore(&cl);
  EXPECT_FALSE(store3->ProxySwitchesAreValid());

  // All proxy switches except no-proxy.
  CommandLine cl2(CommandLine::NO_PROGRAM);
  cl2.AppendSwitch(switches::kProxyAutoDetect);
  cl2.AppendSwitchASCII(switches::kProxyServer, "server");
  cl2.AppendSwitchASCII(switches::kProxyPacUrl, "url");
  cl2.AppendSwitchASCII(switches::kProxyBypassList, "list");
  scoped_refptr<TestCommandLinePrefStore> store4 =
      new TestCommandLinePrefStore(&cl2);
  EXPECT_TRUE(store4->ProxySwitchesAreValid());
}

TEST(CommandLinePrefStoreTest, ManualProxyModeInference) {
  CommandLine cl1(CommandLine::NO_PROGRAM);
  cl1.AppendSwitch(unknown_string);
  cl1.AppendSwitchASCII(switches::kProxyServer, "proxy");
  scoped_refptr<TestCommandLinePrefStore> store1 =
      new TestCommandLinePrefStore(&cl1);
  store1->VerifyProxyMode(ProxyPrefs::MODE_FIXED_SERVERS);

  CommandLine cl2(CommandLine::NO_PROGRAM);
  cl2.AppendSwitchASCII(switches::kProxyPacUrl, "proxy");
  scoped_refptr<TestCommandLinePrefStore> store2 =
        new TestCommandLinePrefStore(&cl2);
  store2->VerifyProxyMode(ProxyPrefs::MODE_PAC_SCRIPT);

  CommandLine cl3(CommandLine::NO_PROGRAM);
  cl3.AppendSwitchASCII(switches::kProxyServer, "");
  scoped_refptr<TestCommandLinePrefStore> store3 =
        new TestCommandLinePrefStore(&cl3);
  store3->VerifyProxyMode(ProxyPrefs::MODE_DIRECT);
}

TEST(CommandLinePrefStoreTest, DisableSSLCipherSuites) {
  CommandLine cl1(CommandLine::NO_PROGRAM);
  cl1.AppendSwitchASCII(switches::kCipherSuiteBlacklist,
                        "0x0004,0x0005");
  scoped_refptr<TestCommandLinePrefStore> store1 =
      new TestCommandLinePrefStore(&cl1);
  const char* const expected_ciphers1[] = {
    "0x0004",
    "0x0005",
  };
  store1->VerifySSLCipherSuites(expected_ciphers1,
                                arraysize(expected_ciphers1));

  CommandLine cl2(CommandLine::NO_PROGRAM);
  cl2.AppendSwitchASCII(switches::kCipherSuiteBlacklist,
                        "0x0004, WHITESPACE_IGNORED TEST , 0x0005");
  scoped_refptr<TestCommandLinePrefStore> store2 =
      new TestCommandLinePrefStore(&cl2);
  const char* const expected_ciphers2[] = {
    "0x0004",
    "WHITESPACE_IGNORED TEST",
    "0x0005",
  };
  store2->VerifySSLCipherSuites(expected_ciphers2,
                                arraysize(expected_ciphers2));

  CommandLine cl3(CommandLine::NO_PROGRAM);
  cl3.AppendSwitchASCII(switches::kCipherSuiteBlacklist,
                        "0x0004;MOAR;0x0005");
  scoped_refptr<TestCommandLinePrefStore> store3 =
      new TestCommandLinePrefStore(&cl3);
  const char* const expected_ciphers3[] = {
    "0x0004;MOAR;0x0005"
  };
  store3->VerifySSLCipherSuites(expected_ciphers3,
                                arraysize(expected_ciphers3));
}
