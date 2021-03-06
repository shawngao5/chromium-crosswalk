// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/passwords/account_avatar_fetcher_manager.h"

#include "base/memory/weak_ptr.h"
#import "chrome/browser/ui/cocoa/passwords/credential_item_view.h"
#include "chrome/browser/ui/passwords/account_avatar_fetcher.h"
#include "chrome/browser/ui/passwords/manage_passwords_view_utils.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_util_mac.h"

class AccountAvatarFetcherBridge;

@interface AccountAvatarFetcherManager()
- (void)updateAvatar:(NSImage*)image
          fromBridge:(AccountAvatarFetcherBridge*)bridge
             forView:(CredentialItemView*)view;
@end

class AccountAvatarFetcherBridge
    : public AccountAvatarFetcherDelegate,
      public base::SupportsWeakPtr<AccountAvatarFetcherBridge> {
 public:
  AccountAvatarFetcherBridge(AccountAvatarFetcherManager* manager,
                             CredentialItemView* view);
  virtual ~AccountAvatarFetcherBridge();

  // AccountAvatarFetcherDelegate:
  void UpdateAvatar(const gfx::ImageSkia& image) override;

 private:
  AccountAvatarFetcherManager* manager_;
  CredentialItemView* view_;
};

AccountAvatarFetcherBridge::AccountAvatarFetcherBridge(
    AccountAvatarFetcherManager* manager,
    CredentialItemView* view)
    : manager_(manager), view_(view) {
}

AccountAvatarFetcherBridge::~AccountAvatarFetcherBridge() = default;

void AccountAvatarFetcherBridge::UpdateAvatar(const gfx::ImageSkia& image) {
  [manager_
      updateAvatar:gfx::NSImageFromImageSkia(ScaleImageForAccountAvatar(image))
        fromBridge:this
           forView:view_];
}

@implementation AccountAvatarFetcherManager

- (id)initWithRequestContext:
        (scoped_refptr<net::URLRequestContextGetter>)requestContext {
  if ((self = [super init])) {
    requestContext_ = requestContext;
  }
  return self;
}

- (void)startRequestWithFetcher:(AccountAvatarFetcher*)fetcher {
  fetcher->Start(requestContext_.get());
}

- (void)fetchAvatar:(const GURL&)avatarURL forView:(CredentialItemView*)view {
  scoped_ptr<AccountAvatarFetcherBridge> bridge(
      new AccountAvatarFetcherBridge(self, view));
  AccountAvatarFetcher* fetcher =
      new AccountAvatarFetcher(avatarURL, bridge->AsWeakPtr());
  bridges_.push_back(bridge.Pass());
  [self startRequestWithFetcher:fetcher];
}

- (void)updateAvatar:(NSImage*)image
          fromBridge:(AccountAvatarFetcherBridge*)bridge
             forView:(CredentialItemView*)view {
  [view updateAvatar:image];
  auto it = std::find(bridges_.begin(), bridges_.end(), bridge);
  if (it != bridges_.end())
    bridges_.erase(it);
}

@end
