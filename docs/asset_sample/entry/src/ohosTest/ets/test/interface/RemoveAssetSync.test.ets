import { asset } from '@kit.AssetStoreKit';
import { stringToArray,
  addAsset,
  removeAsset,
  removeAssetError,
  removeAssetParamNumError
} from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';


export default function removeAssetSyncTest() {
  describe('removeAssetSyncTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('removeSingleData', 0, () => {
      let alias = 'removeSingleData'
      expect(addAsset(alias)).assertTrue()
      expect(removeAsset(alias)).assertTrue()
    })

    it('removeNoeFoundData', 0, () => {
      let alias = 'removeSingleData'
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias))
      expect(removeAssetError(attr)).assertDeepEquals(asset.ErrorCode.NOT_FOUND)
    })

    it('removeInputParamError', 0, () => {
      expect(removeAssetParamNumError()).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('removeInputMapKeyError', 0, () => {
      let alias = 'removeInputMapKeyError'
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias));
      attr.set(asset.Tag.SECRET, 12);
      expect(removeAssetError(attr)).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })

  })
}