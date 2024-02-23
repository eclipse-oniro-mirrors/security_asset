import { asset } from '@kit.AssetStoreKit';
import { stringToArray,
  removeAsset,
  addAssetPromiseResult,
  preQueryAsset,
  postQueryAsset,
  preQueryAssetResult,
  preQueryAssetParamNumError
} from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';


export default function preQueryAssetSyncTest() {
  describe('preQueryAssetSyncTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })

    it('preQuerySingleData', 0, async () => {
      let attr: asset.AssetMap = new Map()
      let alias = 'preQuerySingleData'
      attr.set(asset.Tag.ALIAS, stringToArray(alias))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      attr.delete(asset.Tag.SECRET)
      let challenge: Uint8Array = preQueryAsset(attr)
      expect(challenge.length).assertEqual(32)
      expect(postQueryAsset(challenge)).assertTrue()
      expect(removeAsset(alias)).assertTrue()
    })

    it('preQueryNotFoundData', 0, () => {
      let attr: asset.AssetMap = new Map();
      expect(preQueryAssetResult(attr)).assertEqual(asset.ErrorCode.NOT_FOUND)
    })

    it('preQueryInputParamError', 0, () => {
      expect(preQueryAssetParamNumError()).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('preQueryInputMapKeyError', 0, () => {
      let alias = 'addInputMapKeyError'
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias));
      attr.set(asset.Tag.SECRET, 12);
      expect(preQueryAssetResult(attr)).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })


  })
}