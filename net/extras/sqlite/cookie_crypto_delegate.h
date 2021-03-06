// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_EXTRAS_SQLITE_COOKIE_CRYPTO_DELEGATE_H_
#define NET_EXTRAS_SQLITE_COOKIE_CRYPTO_DELEGATE_H_

namespace net {

// Implements encryption and decryption for the persistent cookie store.
class CookieCryptoDelegate {
 public:
  virtual ~CookieCryptoDelegate() {}
  virtual bool EncryptString(const std::string& plaintext,
                             std::string* ciphertext) = 0;
  virtual bool DecryptString(const std::string& ciphertext,
                             std::string* plaintext) = 0;
};

}  // namespace net

#endif  // NET_EXTRAS_SQLITE_COOKIE_CRYPTO_DELEGATE_H_
