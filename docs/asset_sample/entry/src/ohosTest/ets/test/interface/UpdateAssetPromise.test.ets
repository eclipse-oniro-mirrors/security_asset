import asset from '@ohos.security.asset';
import { stringToArray, removeAssetPromise, addAssetPromise,
  queryContentPromise, queryAttrPromise,
  updateAssetByAlias,
  updateQueriedAsset
} from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

export default function updateAssetPromiseTest() {
  describe('updateFuncPromiseTest', () => {
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
      expect(await addAssetPromise(alias)).assertTrue()

      let update: asset.AssetMap = new Map()
      update.set(asset.Tag.SECRET, stringToArray(alias))
      expect(await updateAssetByAlias(alias, update)).assertEqual(0)

      let res: asset.AssetMap = await queryContentPromise(alias)
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray(alias))
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('updateDataLabel', 0, async () => {
      let alias = 'updateDataLabel'
      expect(await addAssetPromise(alias)).assertTrue()

      let update: asset.AssetMap = new Map()
      update.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray(alias))
      expect(await updateAssetByAlias(alias, update)).assertEqual(0)

      let res: asset.AssetMap = await queryAttrPromise(alias)
      expect(res.get(asset.Tag.DATA_LABEL_NORMAL_1)).assertDeepEquals(stringToArray(alias))
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('updateDataNotExist', 0, async () => {
      let alias = 'updateDataNotExist'
      let update: asset.AssetMap = new Map()
      update.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray(alias))
      expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.NOT_FOUND)
    })
  })

  describe('updateInvalidQueryPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('queryWithoutAlias', 0, async () => {
      let query: asset.AssetMap = new Map();
      expect(await updateQueriedAsset(query, 'queryWithoutAlias')).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidAliasLen', 0, async () => {
      let query: asset.AssetMap = new Map();
      query.set(asset.Tag.ALIAS, new Uint8Array(257));
      expect(await updateQueriedAsset(query, 'queryInvalidAliasLen')).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.ALIAS, new Uint8Array(0));
      expect(await updateQueriedAsset(query, 'queryInvalidAliasLen')).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryUnmatchedAliasType', 0, async () => {
      let query: asset.AssetMap = new Map();
      query.set(asset.Tag.ALIAS, 0);
      expect(await updateQueriedAsset(query, 'queryUnmatchedAliasType')).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.ALIAS, false);
      expect(await updateQueriedAsset(query, 'queryUnmatchedAliasType')).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidAccessibility', 0, async () => {
      let alias = 'queryInvalidAccessibility'
      let query: asset.AssetMap = new Map();
      query.set(asset.Tag.ALIAS, stringToArray(alias));
      query.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON - 1)
      expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_UNLOCKED + 1)
      expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidAuthType', 0, async () => {
      let alias = 'queryInvalidAuthType'
      let query: asset.AssetMap = new Map();
      query.set(asset.Tag.ALIAS, stringToArray(alias));
      query.set(asset.Tag.AUTH_TYPE, asset.AuthType.NONE + 1)
      expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY + 1)
      expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidSyncType', 0, async () => {
      let alias = 'queryInvalidSyncType'
      let query: asset.AssetMap = new Map();
      query.set(asset.Tag.ALIAS, stringToArray(alias));
      query.set(asset.Tag.SYNC_TYPE, (asset.SyncType.THIS_DEVICE | asset.SyncType.TRUSTED_DEVICE) + 1)
      expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.SYNC_TYPE, asset.SyncType.NEVER - 1)
      expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidLabelLen', 0, async () => {
      let alias = 'queryInvalidLabelLen'
      let labels = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4
      ]
      for (let i = 0; i < labels.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(asset.Tag.ALIAS, stringToArray(alias))
        query.set(labels[i], new Uint8Array(0))
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(labels[i], new Uint8Array(513))
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('queryBoolUnmatchedType', 0, async () => {
      let alias = 'queryBoolUnmatchedType'
      let tags = [
        asset.Tag.REQUIRE_PASSWORD_SET, asset.Tag.IS_PERSISTENT
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(asset.Tag.ALIAS, stringToArray(alias))
        query.set(tags[i], new Uint8Array(1))
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], 0)
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('queryBytesUnmatchedType', 0, async () => {
      let alias = 'queryBytesUnmatchedType'
      let tags = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(asset.Tag.ALIAS, stringToArray(alias))
        query.set(tags[i], 0)
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], false)
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('queryNumberUnmatchedType', 0, async () => {
      let alias = 'queryNumberUnmatchedType'
      let tags = [
        asset.Tag.ACCESSIBILITY, asset.Tag.AUTH_TYPE, asset.Tag.SYNC_TYPE
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(asset.Tag.ALIAS, stringToArray(alias))
        query.set(tags[i], new Uint8Array(1))
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], false)
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('queryUnsupportedTag', 0, async () => {
      let alias = 'queryUnsupportedTag'
      let tags = [ asset.Tag.SECRET, asset.Tag.AUTH_CHALLENGE, asset.Tag.AUTH_TOKEN]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(asset.Tag.ALIAS, stringToArray(alias))
        query.set(tags[i], new Uint8Array(1))
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }

      let numberTags = [asset.Tag.AUTH_VALIDITY_PERIOD, asset.Tag.CONFLICT_RESOLUTION, asset.Tag.RETURN_TYPE,
        asset.Tag.RETURN_LIMIT, asset.Tag.RETURN_OFFSET, asset.Tag.RETURN_ORDERED_BY]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(asset.Tag.ALIAS, stringToArray(alias))
        query.set(numberTags[i], 1)
        expect(await updateQueriedAsset(query, alias)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })
  })

  describe('updateInvalidUpdatePromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('updateEmptyAttr', 0, async () => {
      let alias = 'updateEmptyAttr'
      let update: asset.AssetMap = new Map()
      expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('updateInvalidSecretLen', 0, async () => {
      let alias = 'updateInvalidSecretLen'
      let update: asset.AssetMap = new Map()
      update.set(asset.Tag.SECRET, new Uint8Array(0))
      expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      update.set(asset.Tag.SECRET, new Uint8Array(1025))
      expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('updateInvalidLabelLen', 0, async () => {
      let alias = 'updateInvalidLabelLen'
      let labels = [
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4
      ]
      for (let i = 0; i < labels.length; i++) {
        let update: asset.AssetMap = new Map()
        update.set(labels[i], new Uint8Array(0))
        expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        update.set(labels[i], new Uint8Array(513))
        expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('updateBytesUnmatchedType', 0, async () => {
      let alias = 'updateBytesUnmatchedType'
      let tags = [
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4, asset.Tag.SECRET
      ]
      for (let i = 0; i < tags.length; i++) {
        let update: asset.AssetMap = new Map()
        update.set(tags[i], 0)
        expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        update.set(tags[i], false)
        expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('updateUnsupportedTag', 0, async () => {
      let alias = 'updateUnsupportedTag'
      let tags = [
        asset.Tag.ALIAS, asset.Tag.AUTH_CHALLENGE, asset.Tag.AUTH_TOKEN,
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4
      ]
      for (let i = 0; i < tags.length; i++) {
        let update: asset.AssetMap = new Map()
        update.set(tags[i], new Uint8Array(1))
        expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }

      let numberTags = [
        asset.Tag.AUTH_VALIDITY_PERIOD, asset.Tag.CONFLICT_RESOLUTION, asset.Tag.ACCESSIBILITY,
        asset.Tag.RETURN_LIMIT, asset.Tag.RETURN_OFFSET, asset.Tag.RETURN_ORDERED_BY, asset.Tag.RETURN_TYPE,
        asset.Tag.AUTH_TYPE, asset.Tag.SYNC_TYPE
      ]
      for (let i = 0; i < numberTags.length; i++) {
        let update: asset.AssetMap = new Map()
        update.set(numberTags[i], 1)
        expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }

      let boolTags = [asset.Tag.REQUIRE_PASSWORD_SET, asset.Tag.IS_PERSISTENT]
      for (let i = 0; i < boolTags.length; i++) {
        let update: asset.AssetMap = new Map()
        update.set(boolTags[i], false)
        expect(await updateAssetByAlias(alias, update)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })
  })
}