import { asset } from '@kit.AssetStoreKit';
import { stringToArray,
  addAsset,
  removeAsset,
  updateAssetByAliasSync,
  queryContentPromise,
  updateAssetParamNumError,
  updateAssetSync
} from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';


export default function updateAssetSyncTest() {
  describe('updateAssetSyncTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })

    it('updateSingleData', 0, async () => {
      let alias = 'updateSingleData'
      expect(addAsset(alias)).assertTrue()

      let update: asset.AssetMap = new Map()
      update.set(asset.Tag.SECRET, stringToArray(alias))
      expect(updateAssetByAliasSync(alias, update)).assertEqual(0)

      let res: asset.AssetMap = await queryContentPromise(alias)
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray(alias))
      expect(removeAsset(alias)).assertTrue()
    })

    it('updateNotFoundData', 0, () => {
      let alias = 'updateSingleData'
      let update: asset.AssetMap = new Map()
      update.set(asset.Tag.SECRET, stringToArray(alias))
      expect(updateAssetByAliasSync(alias, update)).assertDeepEquals(asset.ErrorCode.NOT_FOUND)
    })

    it('updateInputParamError', 0, () => {
      expect(updateAssetParamNumError('updateInputParamError')).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('updateMapKeyError1', 0, () => {
      let alias = 'updateMapKeyError1'
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias));
      attr.set(asset.Tag.SECRET, 12);

      let update: asset.AssetMap = new Map()
      update.set(asset.Tag.SECRET, stringToArray(alias))
      expect(updateAssetSync(attr, update)).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('updateMapKeyError2', 0, () => {
      let alias = 'updateMapKeyError2'
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray(alias));
      attr.set(asset.Tag.SECRET, stringToArray(alias));

      let update: asset.AssetMap = new Map()
      update.set(asset.Tag.SECRET, 12);
      expect(updateAssetSync(attr, update)).assertDeepEquals(asset.ErrorCode.INVALID_ARGUMENT)
    })


  })
}