import { asset } from '@kit.AssetStoreKit';
import { stringToArray,
  addAsset,
  queryContent,
  removeAsset,
  queryCount,
  queryError,
  queryAssetParamNumError
} from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';


export default function queryAssetSyncTest() {
  describe('queryAssetSyncTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })

    it('querySingleData', 0, () => {
      let alias = 'querySingleData'
      expect(addAsset(alias)).assertTrue()
      let res = queryContent(alias)
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray("demo_pwd"))
      expect(removeAsset(alias)).assertTrue()
    })

    it('queryMultipleData', 0, () => {
      let alias = 'queryMultipleData'
      expect(addAsset(alias)).assertTrue()
      let alias2 = 'queryMultipleData2'
      expect(addAsset(alias2)).assertTrue()
      expect(queryCount()).assertDeepEquals(2)
      expect(removeAsset(alias)).assertTrue()
      expect(removeAsset(alias2)).assertTrue()
    })

    it('queryNotExistData', 0, () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray('queryNotExistData'));
      expect(queryError(attr)).assertEqual(asset.ErrorCode.NOT_FOUND)
    })

    it('queryInputParamError', 0, () => {
      expect(queryAssetParamNumError()).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('addInputMapKeyError', 0, () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, 12);
      expect(queryError(attr)).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })

  })
}