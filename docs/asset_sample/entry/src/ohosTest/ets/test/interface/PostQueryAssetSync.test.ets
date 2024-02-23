import { asset } from '@kit.AssetStoreKit';
import { stringToArray,
  removeAsset,
  addAssetResult,
  postQueryAsset,
  preQueryAsset,
  postQueryAssetParamNumError,
  postQueryAssetResult
} from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';


export default function postQueryAssetSyncTest() {
  describe('postQueryAssetSyncTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })

    it('postQuerySingleData', 0, () => {
      let attr: asset.AssetMap = new Map()
      let alias = 'postQuerySingleData'
      attr.set(asset.Tag.ALIAS, stringToArray(alias))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      expect(addAssetResult(attr)).assertEqual(0)
      attr.delete(asset.Tag.SECRET)
      let challenge: Uint8Array = preQueryAsset(attr)
      expect(challenge.length).assertEqual(32)
      expect(postQueryAsset(challenge)).assertTrue()
      expect(removeAsset(alias)).assertTrue()
    })

    it('postQueryNotExistData', 0, async () => {
      expect(postQueryAsset(new Uint8Array(32))).assertTrue()
    })

    it('postQueryInputParamError', 0, () => {
      expect(postQueryAssetParamNumError()).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('postQueryInputMapKeyError', 0, () => {
      let alias = 'postQueryInputMapKeyError'
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias));
      attr.set(asset.Tag.SECRET, 12);
      expect(postQueryAssetResult(attr)).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })

  })
}